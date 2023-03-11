
#include "pch.h"
#include "BaseServer.h"
#include "RConServer.h"
#include <mongoose.h>

using namespace Galactic3D;

class CRConLogger : public Logger
{
public:
	CRConLogger(CRConServer* pServer, mg_connection* pConnection);
	virtual ~CRConLogger();

	CRConServer* m_pServer;
	mg_connection* m_pConnection;

	virtual void Log(eLogType LogType, const GChar* pszMessage) override;
};

CRConLogger::CRConLogger(CRConServer* pServer, mg_connection* pConnection) : m_pServer(pServer), m_pConnection(pConnection)
{
	pServer->m_pServer->m_ResourceMgr.m_pCommandHandlers->PushLogger(this);
}

CRConLogger::~CRConLogger()
{
	m_pServer->m_pServer->m_ResourceMgr.m_pCommandHandlers->PopLogger();
}

void CRConLogger::Log(eLogType LogType, const GChar* pszMessage)
{
	m_pServer->Log(m_pConnection, LogType, pszMessage);
}

CRConServer::CRConServer(CBaseServer* pServer) : m_pServer(pServer)
{
}

void CRConServer::ProcessPacket(mg_connection* nc, unsigned int PacketID, Stream* pStream)
{
	switch (PacketID)
	{
		case 1: // COMMAND
			{
				if (nc->user_data == (void*)(size_t)NETGAME_SECURITY_KEY)
				{
					CRConLogger Logger(this, nc);

					CBinaryReader Reader(pStream);
					size_t Length;
					auto pszCommand = Reader.ReadString(&Length);

					if (!m_pServer->m_ResourceMgr.m_pCommandHandlers->ConsoleCommand(pszCommand, nullptr, true))
					{
						GString Command = pszCommand;
						GString Text = _gstr("'");
						GString CommandString;
						size_t Offset = Command.find(_gstr(' '));
						if (Offset != GString::npos)
							CommandString = Command.substr(0, Offset);
						else
							CommandString = Command;
						Text += CommandString;
						Text += _gstr("' is not recognized as an internal or external command, operable program or batch file.");
						Logger.LogFormatted(LOGTYPE_WARN, Text.c_str());
					}

					GFree(pszCommand);
				}
				else
					Log(nc, LOGTYPE_ERROR, _gstr("Not authenticated"));
			}
			break;

		case 2: // AUTH
			{
				CBinaryReader Reader(pStream);

				size_t PasswordLength;
				auto pszPassword = Reader.ReadString(&PasswordLength);

				if (m_Password.Verify(pszPassword))
				{
					nc->user_data = (void*)(size_t)NETGAME_SECURITY_KEY;
					Auth(nc, 1);
				}
				else
					Auth(nc, 0);

				GFree(pszPassword);
			}
			break;

		default:
			break;
	}
}

void CRConServer::EventHandler(struct mg_connection* nc, int ev, void* ev_data)
{
	if (ev == MG_EV_HTTP_REQUEST)
		m_pServer->m_HttpServer.HTTPHandler(nc,ev,ev_data);
	else
		WebConnection::EventHandler(nc,ev,ev_data);
}

void CRConServer::Log(mg_connection * nc, eLogType LogType, const GChar* psz)
{
	Packet Packet(1);
	CBinaryWriter Writer(&Packet);

	Packet.Write<uint8_t>((uint8_t)LogType);
	Writer.WriteString(psz, _gstrlen(psz));

	SendPacket(nc, &Packet);
}

void CRConServer::Auth(mg_connection* nc, bool b)
{
	Packet Packet(2);

	Packet.Write<uint8_t>(b ? 1 : 0);

	SendPacket(nc, &Packet);
}
