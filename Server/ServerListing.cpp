#include "pch.h"
#include "Server.h"
#include "ServerListing.h"
#include <mongoose.h>

CMasterlistAnnouncer::CMasterlistAnnouncer(CServer* pServer)
	: m_pServer(pServer)
{
	m_State = MASTERLIST_NONE;
	m_fRetryTime = 0.0f;
}

void CMasterlistAnnouncer::ChangeState(eMasterlistState State)
{
	if (m_State != State)
	{
		m_OldState = m_State;
		m_State = State;
	}
}

void CMasterlistAnnouncer::Pulse(float fDeltaTime)
{
	if (m_State == MASTERLIST_DEAD)
		return;

	if (m_State == MASTERLIST_DISCONNECTED)
	{
		if (IsActive())
			StopServer();

		if (m_OldState == MASTERLIST_CONNECTING)
			_glogerrorprintf(_gstr("Failed to connect to the server listing"));
		else
			_glogprintf(_gstr("Disconnected from server listing"));

		ChangeState(MASTERLIST_NONE);

		return;
	}
	else if (m_State == MASTERLIST_NONE)
	{
		if (m_fRetryTime <= 0.0f)
		{
			m_fRetryTime = 5.0f;

			if (IsActive())
				StopServer();

			if (m_pServer->StartMasterlist())
				ChangeState(MASTERLIST_CONNECTING);
			else
				m_fRetryTime = 30.0f;
		}
		else
		{
			m_fRetryTime -= fDeltaTime;
			if (m_fRetryTime < 0.0f)
				m_fRetryTime = 0.0f;

			return;
		}
	}

	UpdateNetwork();
}

void CMasterlistAnnouncer::EventHandler(struct mg_connection* nc, int ev, void* ev_data)
{
	if (ev == MG_EV_WEBSOCKET_FRAME)
	{
		struct websocket_message *wm = (struct websocket_message *) ev_data;

		CMemoryStream Stream(wm->data, wm->size, true, false);
		{
			CBinaryReader Reader(&Stream);

			Sint32 packetIdentifier;
			if (!Reader.Read7BitEncodedInt(&packetIdentifier))
				return;

			ProcessPacket(nc, (Uint32)packetIdentifier, &Stream);
		}
	}
	else
		WebConnection::EventHandler(nc, ev, ev_data);
}

void CMasterlistAnnouncer::ProcessPacket(mg_connection* nc, unsigned int PacketID, Stream* pStream)
{
	switch (PacketID)
	{
		case 4: // JOINED
			break;
		case 5: // LOG
			{
				CBinaryReader Reader(pStream);

				int32_t LogType;
				if (!Reader.Read7BitEncodedInt(&LogType))
					return;

				size_t Length;
				auto pszMessage = Reader.ReadString(&Length);
				if (pszMessage == nullptr)
					return;

				_glogmessageprintf((eLogType)LogType, _gstr("%s"), pszMessage);

				GFree(pszMessage);
			}
			break;
		case 6: // JOINERROR
			break;
		default:
			break;
	}
}

enum eMasterListClientPacket
{
	MASTERLISTCLIENTPACKET_JOIN,

	MASTERLISTCLIENTPACKET_PLAYERADD,
	MASTERLISTCLIENTPACKET_PLAYERREMOVE,

	MASTERLISTCLIENTPACKET_SETGAMEMODE,
	MASTERLISTCLIENTPACKET_SETNAME,
};

void CMasterlistAnnouncer::PlayerAdd(CNetMachine* pNetMachine)
{
	if (m_pHTTPServer == nullptr)
		return;

	Packet Packet;
	CBinaryWriter Writer(&Packet);

	Writer.Write7BitEncodedInt(MASTERLISTCLIENTPACKET_PLAYERADD);

	Writer.Write7BitEncodedInt(pNetMachine->m_nIndex);
	Writer.WriteString(pNetMachine->GetName(), pNetMachine->GetNameLength());
	Writer.WriteString(pNetMachine->m_Game.c_str(), pNetMachine->m_Game.length());

	SendPacket(&Packet);
}

void CMasterlistAnnouncer::PlayerRemove(CNetMachine* pNetMachine)
{
	if (m_pHTTPServer == nullptr)
		return;

	Packet Packet;
	CBinaryWriter Writer(&Packet);

	Writer.Write7BitEncodedInt(MASTERLISTCLIENTPACKET_PLAYERREMOVE);

	Writer.Write7BitEncodedInt(pNetMachine->m_nIndex);

	SendPacket(&Packet);
}

void CMasterlistAnnouncer::SetGameMode(const GChar* pszGameMode, size_t Length)
{
	if (m_pHTTPServer == nullptr)
		return;

	Packet Packet;
	CBinaryWriter Writer(&Packet);

	Writer.Write7BitEncodedInt(MASTERLISTCLIENTPACKET_SETGAMEMODE); // JOIN

	Writer.WriteString(pszGameMode, Length);

	SendPacket(&Packet);
}

void CMasterlistAnnouncer::SetName(const GChar* pszName, size_t Length)
{
	if (m_pHTTPServer == nullptr)
		return;

	Packet Packet;
	CBinaryWriter Writer(&Packet);

	Writer.Write7BitEncodedInt(MASTERLISTCLIENTPACKET_SETNAME);

	Writer.WriteString(pszName, Length);

	SendPacket(&Packet);
}

enum eMasterlistJoinFlags
{
	MASTERLISTJOINFLAGS_SERVER = 1,
	MASTERLISTJOINFLAGS_LOCKED = 2,
	MASTERLISTJOINFLAGS_CONSOLE = 4,
	MASTERLISTJOINFLAGS_NETVERSION = 8,
};

void CMasterlistAnnouncer::OnPlayerConnect(mg_connection* nc)
{
	Packet Packet;
	CBinaryWriter Writer(&Packet);

	Writer.Write7BitEncodedInt(0); // JOIN
	int32_t Flags = MASTERLISTJOINFLAGS_SERVER | MASTERLISTJOINFLAGS_NETVERSION;;
	if (m_pServer->m_pConsole != nullptr)
		Flags |= MASTERLISTJOINFLAGS_CONSOLE;
	if (m_pServer->m_Password.HasPassword())
		Flags |= MASTERLISTJOINFLAGS_LOCKED; // Locked
	Writer.Write7BitEncodedInt(Flags);

	Writer.Write7BitEncodedInt((Sint32)1); // Count
	Writer.WriteString(m_pServer->m_Game.c_str(), m_pServer->m_Game.length());

#ifdef _DEBUG
	if (m_pServer->m_uiFakeNetVersion != 0)
		Writer.Write7BitEncodedInt((int32_t)m_pServer->m_uiFakeNetVersion);
	else
		Writer.Write7BitEncodedInt((int32_t)m_pServer->m_uiNetVersion);
#else
	Writer.Write7BitEncodedInt((int32_t)m_pServer->m_uiNetVersion);
#endif

	Writer.Write7BitEncodedInt((Sint32)m_pServer->m_usPort);
	Writer.Write7BitEncodedInt((Sint32)m_pServer->m_MaxClients);
	Writer.Write7BitEncodedInt((Sint32)m_pServer->m_CurrentClients);
	for (size_t i = 0; i < MAX_MACHINES; i++)
	{
		auto pNetMachine = m_pServer->m_NetMachines.GetMachine(i);
		if (pNetMachine != nullptr)
		{
			Writer.Write7BitEncodedInt((Sint32)i);
			Writer.WriteString(pNetMachine->GetName(), pNetMachine->GetNameLength());
			Writer.WriteString(pNetMachine->m_Game.c_str(), pNetMachine->m_Game.length());
		}
	}
	Writer.WriteString(m_pServer->m_szServerName, _gstrlen(m_pServer->m_szServerName));
	Writer.WriteString(m_pServer->GetGameMode(), _gstrlen(m_pServer->GetGameMode()));

	Writer.Write7BitEncodedInt((int32_t)m_pServer->m_uiMinMajorVersion);
	Writer.Write7BitEncodedInt((int32_t)m_pServer->m_uiMinMinorVersion);
	Writer.Write7BitEncodedInt((int32_t)m_pServer->m_uiMinPatchVersion);

	SendPacket(&Packet);

	ChangeState(MASTERLIST_CONNECTED);

	_glogprintf(_gstr("Connected to the server listing"));
}

void CMasterlistAnnouncer::OnPlayerDisconnect(mg_connection *nc)
{
	if (m_State != MASTERLIST_NONE)
	{
		ChangeState(MASTERLIST_DISCONNECTED);
	}
}