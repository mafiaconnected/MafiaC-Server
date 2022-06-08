
#include "pch.h"
#include "Server.h"
#include "UnleashedGameProtocolHandler.h"
#include <Multiplayer/NetworkHandlers.h>
#include <time.h>
#ifndef WIN32
#include <sys/stat.h>
#else
#include <direct.h>
#endif
#include <RakNet/SecureHandshake.h>
#include <mongoose.h>
#include <iostream>
#include <string>
#include <sstream>
#include <zlib.h>
#include "MafiaServerManager.h"

using namespace Galactic3D;

extern bool RegisterLuaVM(CScripting* pScripting);
extern bool RegisterJSVM(CScripting* pScripting);
extern bool RegisterSqVM(CScripting* pScripting);

class CRConLogger : public Logger
{
public:
	CRConLogger(CRConServer* pServer, mg_connection* pConnection);
	virtual ~CRConLogger(void);

	CRConServer* m_pServer;
	mg_connection* m_pConnection;

	virtual void Log(eLogType LogType, const GChar* pszMessage) override;
};

CRConLogger::CRConLogger(CRConServer* pServer, mg_connection* pConnection) : m_pServer(pServer), m_pConnection(pConnection)
{
	pServer->m_pServer->m_ResourceMgr.m_pCommandHandlers->PushLogger(this);
}

CRConLogger::~CRConLogger(void)
{
	m_pServer->m_pServer->m_ResourceMgr.m_pCommandHandlers->PopLogger();
}

void CRConLogger::Log(eLogType LogType, const GChar* pszMessage)
{
	m_pServer->Log(m_pConnection, LogType, pszMessage);
}

CRConServer::CRConServer(CServer* pServer) : m_pServer(pServer)
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

class CExitCommandHandler : public CCommandHandler
{
public:
	CExitCommandHandler(CServer* pServer) : m_pServer(pServer)
	{
	}

	CServer* m_pServer;

	virtual void Execute(const GChar* pszCommandName, const GChar* pszArguments, CBaseObject* pClient) override
	{
		m_pServer->m_bExit = true;
		m_pServer->m_ExitSignal.Signal();
	}
};

class CSayCommandHandler : public CCommandHandler
{
public:
	CSayCommandHandler(CServer* pServer) : m_pServer(pServer)
	{
	}

	CServer* m_pServer;

	virtual void Execute(const GChar* pszCommandName, const GChar* pszArguments, CBaseObject* pClient) override;
};

void CSayCommandHandler::Execute(const GChar* pszCommandName, const GChar* pszArguments, CBaseObject* pClient)
{
	if (pszArguments == nullptr)
		pszArguments = _gstr("");

	CNetMachine* pClient2 = static_cast<CNetMachine*>(pClient);
	assert(pClient2);
	if (pClient2 != nullptr)
		m_pServer->UserChat(pClient2, pszArguments, _gstrlen(pszArguments));
}

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

CServerConfiguration::CServerConfiguration()
{
	m_pConfig = nullptr;
}

CServerConfiguration::~CServerConfiguration()
{
	if (m_pConfig != nullptr)
		delete m_pConfig;
}

bool CServerConfiguration::GetBoolValue(const GChar* pszName, bool bDefault) const
{
	auto pElement = m_pConfig->FirstChildElement(pszName);
	if (pElement != nullptr)
	{
		const GChar* pszText = pElement->GetText();
		if (pszText != nullptr)
			return StringManager::ToBoolean(pszText, bDefault);
	}
	return bDefault;
}

const GChar* CServerConfiguration::GetStringValue(const GChar* pszName, const GChar* pszDefault) const
{
	auto pElement = m_pConfig->FirstChildElement(pszName);
	if (pElement != nullptr)
	{
		const GChar* pszText = pElement->GetText();
		if (pszText != nullptr)
			return pszText;
	}
	return pszDefault;
}

int32_t CServerConfiguration::GetInt32Value(const GChar* pszName, int32_t iDefault) const
{
	auto pElement = m_pConfig->FirstChildElement(pszName);
	if (pElement != nullptr)
	{
		const GChar* pszText = pElement->GetText();
		if (pszText != nullptr)
			return _gstrtol(pszText, nullptr, 10);
	}
	return iDefault;
}

float CServerConfiguration::GetFloatValue(const GChar* pszName, float fDefault) const
{
	auto pElement = m_pConfig->FirstChildElement(pszName);
	if (pElement != nullptr)
	{
		const GChar* pszText = pElement->GetText();
		if (pszText != nullptr)
			return _gstrtof(pszText, nullptr);
	}
	return fDefault;
}

bool CServerConfiguration::Read(Stream* pStream)
{
	CElementChunk* pConfig = CElementChunk::Load(pStream);
	if (pConfig != nullptr)
	{
		m_pConfig = pConfig;
		return true;
	}
	return false;
}

void CUnhandledCommandHandler2::Execute(const GChar* pszCommandName, const GChar* pszArguments, CBaseObject* pClient)
{
	auto pClient2 = static_cast<CNetMachine*>(pClient);
	if (pClient2 == nullptr || pClient2->m_bConsole)
		CUnhandledCommandHandler::Execute(pszCommandName, pszArguments, pClient);
}

static void FinaliseInputEvent(void* pEvent)
{
	_gstrfree(pEvent);
}

CServer::CServer(Galactic3D::Context* pContext) :
	m_ResourceMgr(pContext, this),
	m_Announcer(this),
	m_HTTPMgr(pContext),
	m_pInputThread(nullptr),
	m_iWaitEventTimeout(5),
	m_UGP(this),
	m_RCon(this),
	m_NetRPC(m_pNetSystem),
	m_InputEvent(FinaliseInputEvent)
{
	m_MaxClients = 32;
	m_pContext = pContext;
	m_pLog = NULL;
	m_pOnPlayerConnectEventType = m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPlayerConnect"), _gstr("Called when a player is attempting to connect"), 1);
	m_pOnPlayerConnectEventType->m_bCanPreventDefault = true;
	m_pOnPlayerJoinEventType = m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPlayerJoin"), _gstr("Called when a player has connected and is joining"), 1);
	m_pOnPlayerJoinedEventType = m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPlayerJoined"), _gstr("Called when the player has joined the game"), 1);
	m_pOnPlayerQuitEventType = m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPlayerQuit"), _gstr("Called when a player disconnects"), 2);
	m_pOnPlayerChatEventType = m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPlayerChat"), _gstr("Called when a player chats"), 2);
	m_pOnPlayerChatEventType->m_bCanPreventDefault = true;
	m_pOnPlayerCommandEventType = m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPlayerCommand"), _gstr("Called when a player types a command"), 3);
	m_pOnPlayerCommandEventType->m_bCanPreventDefault = true;
	m_pOnProcessEventType = m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnProcess"), _gstr("Called every process"), 1);
	m_pOnServerStartEventType = m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnServerStart"), _gstr("Called when the server finished the start sequence"), 0);
	m_bExit = false;
	m_bServerBrowser = false;
	m_bRCon = false;
	m_bServerQuery = false;
	m_bHttpServer = true;
	m_usPort = 22000;
	m_usHTTPPort = 22000;
	m_usRConPort = 22000;
	m_usSyncInterval = 30;
	m_szLogPath[0] = '\0';
	m_szLogTimeStamp[0] = '\0';
	m_CurrentClients = 0;
	m_ResourceMgr.m_pCommandHandlers->AddCommandHandler(new CSayCommandHandler(this), _gstr("say"));
	m_ResourceMgr.m_pCommandHandlers->AddCommandHandler(new CHelpCommandHandler, _gstr("help"));
	m_ResourceMgr.m_pCommandHandlers->AddCommandHandler(new CExitCommandHandler(this), _gstr("quit"));
	m_ResourceMgr.m_pCommandHandlers->AddCommandHandler(new CExitCommandHandler(this), _gstr("exit"));

	m_ResourceMgr.m_pCommandHandlers->AddCommandHandler(new CStartResourceCommandHandler(&m_ResourceMgr), _gstr("start"));
	m_ResourceMgr.m_pCommandHandlers->AddCommandHandler(new CStopResourceCommandHandler(&m_ResourceMgr), _gstr("stop"));
	m_ResourceMgr.m_pCommandHandlers->AddCommandHandler(new CStopAllResourcesCommandHandler(&m_ResourceMgr), _gstr("stopall"));
	m_ResourceMgr.m_pCommandHandlers->AddCommandHandler(new CRestartResourceCommandHandler(&m_ResourceMgr), _gstr("restart"));
	m_ResourceMgr.m_pCommandHandlers->AddCommandHandler(new CRefreshResourcesCommandHandler(&m_ResourceMgr), _gstr("refresh"));

	m_ResourceMgr.m_pCommandHandlers->AddCommandHandler(new CDumpDocumentationCommandHandler(&m_ResourceMgr), _gstr("dumpdoc"));

	m_ResourceMgr.m_pCommandHandlers->AddUnhandledCmdProc(new CUnhandledCommandHandler2());

	RegisterFunctions(m_ResourceMgr.m_pScripting);

	m_pManager = nullptr;

	m_SyncPacketPriority = PACKETPRIORITY_LOW;
	m_SyncPacketFlags = PACKETFLAGS_NONE;

	m_szGameMode[0] = '\0';
	m_szLevel[0] = '\0';

	m_uiNetVersion = NETGAME_CURRENT_VERSION;

#ifdef _DEBUG
	m_uiFakeNetVersion = 0;
#endif

	RegisterLuaVM(m_ResourceMgr.m_pScripting);
	RegisterJSVM(m_ResourceMgr.m_pScripting);
	RegisterSqVM(m_ResourceMgr.m_pScripting);

	SetRule(_gstr("Version"), __gstr(MAFIAC_SERVER_VERSION));

	m_ServerConfig.RegisterCallback(_gstr("server_name"), this, [](const GChar* pszName, const GChar* pszValue, void* pUser) {
		CServer* pServer = (CServer*)pUser;
		_gstrlcpy(pServer->m_szServerName, pszValue, ARRAY_COUNT(pServer->m_szServerName));
		return true;
	});

	m_ServerConfig.RegisterCallback(_gstr("listen_ip"), this, [](const GChar* pszName, const GChar* pszValue, void* pUser) {
		CServer* pServer = (CServer*)pUser;
		_gstrlcpy(pServer->m_szBindIP, pszValue, ARRAY_COUNT(pServer->m_szBindIP));
		return true;
	});

	m_ServerConfig.RegisterCallback(_gstr("listen_port"), this, [](const GChar* pszName, int32_t iValue, void* pUser) {
		CServer* pServer = (CServer*)pUser;
		pServer->m_usPort = (uint16_t)iValue;
		return true;
	});

	m_ServerConfig.RegisterCallback(_gstr("max_players"), this, [](const GChar* pszName, int32_t iValue, void* pUser) {
		CServer* pServer = (CServer*)pUser;
		pServer->m_MaxClients = (size_t)iValue;
		return true;
	});

	m_ServerConfig.RegisterCallback(_gstr("password"), this, [](const GChar* pszName, const GChar* pszValue, void* pUser) {
		CServer* pServer = (CServer*)pUser;
		pServer->m_Password.SetPassword(pszValue);
		return true;
	});

	m_ServerConfig.RegisterCallback(_gstr("owner"), this, [](const GChar* pszName, const GChar* pszValue, void* pUser) {
		CServer* pServer = (CServer*)pUser;
		pServer->SetRule(_gstr("Owner"), pszValue);
		return true;
	});

	m_ServerConfig.RegisterCallback(_gstr("website"), this, [](const GChar* pszName, const GChar* pszValue, void* pUser) {
		CServer* pServer = (CServer*)pUser;
		pServer->SetRule(_gstr("Website"), pszValue);
		return true;
	});

	m_ServerConfig.RegisterCallback(_gstr("gamemode_name"), this, [](const GChar* pszName, const GChar* pszValue, void* pUser) {
		CServer* pServer = (CServer*)pUser;
		_gstrlcpy(pServer->m_szGameMode, pszValue, ARRAY_COUNT(pServer->m_szGameMode));
		return true;
	});

	m_ServerConfig.RegisterCallback(_gstr("rcon"), this, [](const GChar* pszName, bool bValue, void* pUser) {
		CServer* pServer = (CServer*)pUser;
		pServer->m_bRCon = bValue;
		return true;
	});

	m_ServerConfig.RegisterCallback(_gstr("rcon_port"), this, [](const GChar* pszName, int32_t iValue, void* pUser) {
		CServer* pServer = (CServer*)pUser;
		pServer->m_usRConPort = (uint16_t)iValue;
		return true;
	});

	m_ServerConfig.RegisterCallback(_gstr("rcon_pass"), this, [](const GChar* pszName, const GChar* pszValue, void* pUser) {
		CServer* pServer = (CServer*)pUser;
		pServer->m_RCon.m_Password.SetPassword(pszValue);
		return true;
	});
}

CServer::~CServer(void)
{
	m_InputEvent.Clear();
	m_Announcer.StopServer();
	m_RCon.StopServer();
	m_ResourceMgr.ClearAllResources();
	if (m_pLog != NULL)
	{
		fclose(m_pLog);
		m_pLog = NULL;
	}
	m_HttpServer.StopServer();
	if (m_pManager != NULL)
		delete m_pManager;
}

// Returns true if the request started
static bool FunctionHttpGet(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	const GChar* pszURL = pState->CheckString(0);
	if (!pszURL)
		return false;
	const GChar* pszPostfields = pState->CheckString(1);
	if (!pszPostfields)
		return false;
	CFunction* pWriteCallback = pState->CheckFunction(2);
	if (!pWriteCallback)
		return false;
	CFunction* pCompletedCallback = pState->CheckFunction(3);
	if (!pCompletedCallback)
		return false;
	CHTTPRequestScript* pRequest = new CHTTPRequestScript(&pServer->m_HTTPMgr, pWriteCallback, pCompletedCallback);
	if (pRequest == nullptr)
		return pState->Error(_gstr("Out of memory."));
	pState->ReturnBoolean(pServer->m_HTTPMgr.Download(pszURL,pszPostfields,nullptr,pRequest));
	return true;
}

static bool FunctionMessage(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	size_t MessageLength;
	const GChar* pszMessage = pState->CheckString(0, &MessageLength);
	if (!pszMessage)
		return false;
	unsigned int uiColour = CARGB::Lime;
	if (argc > 1 && !pState->CheckNumber(1, uiColour))
		return false;
	pServer->SendChat(nullptr, pszMessage, MessageLength, uiColour);
	return true;
}

static bool FunctionMessageClient(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	size_t MessageLength;
	const GChar* pszMessage = pState->CheckString(0, &MessageLength);
	if (!pszMessage)
		return false;
	CNetMachine* pClient;
	if (!pState->CheckClass(pServer->m_pManager->m_pNetMachineClass, 1, false, &pClient))
		return false;
	unsigned int uiColour = CARGB::Lime;
	if (argc > 2 && !pState->CheckNumber(2, uiColour))
		return false;
	pServer->SendChat(pClient, pszMessage, MessageLength, uiColour);
	return true;
}

static bool FunctionMessageAllExcept(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	size_t MessageLength;
	const GChar* pszMessage = pState->CheckString(0, &MessageLength);
	if (!pszMessage)
		return false;
	CNetMachine* pClient;
	if (!pState->CheckClass(pServer->m_pManager->m_pNetMachineClass, 1, false, &pClient))
		return false;
	unsigned int uiColour = CARGB::Lime;
	if (argc > 2 && !pState->CheckNumber(2, uiColour))
		return false;
	pServer->SendChatExcept(pClient, pszMessage, MessageLength, uiColour);
	return true;
}

static bool FunctionGetServerGame(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	pState->ReturnNumber(pServer->m_GameId);
	return true;
}

static bool FunctionGetServerName(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	pState->ReturnString(pServer->m_szServerName);
	return true;
}

static bool FunctionSetServerName(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	const GChar* pszName = pState->CheckString(0);
	if (!pszName)
		return false;
	pServer->SetServerName(pszName);
	return true;
}

static bool FunctionGetGameMode(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	pState->ReturnString(pServer->GetGameMode());
	return true;
}

static bool FunctionSetGameMode(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	const GChar* pszGameMode = pState->CheckString(0);
	if (!pszGameMode)
		return false;
	pServer->SetGameMode(pszGameMode);
	return true;
}

static bool FunctionGetServerPort(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	pState->ReturnNumber(pServer->m_usPort);
	return true;
}

static bool FunctionGetServerMaxClients(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	pState->ReturnNumber(pServer->m_MaxClients);
	return true;
}

static bool FunctionServerSetRule(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	const GChar* pszKey = pState->CheckString(0);
	if (!pszKey)
		return false;
	const GChar* pszValue = pState->CheckString(1);
	if (!pszValue)
		return false;
	pServer->SetRule(pszKey, pszValue);
	return true;
}

static bool FunctionServerGetRule(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	const GChar* pszKey = pState->CheckString(0);
	if (!pszKey)
		return false;
	pState->ReturnString(pServer->GetRule(pszKey));
	return true;
}

static bool FunctionServerBanIP(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	const GChar* pszIP = pState->CheckString(0);
	if (!pszIP)
		return false;
	uint32_t uiMilliseconds;
	if (!pState->CheckNumber(1, uiMilliseconds))
		return false;
	pServer->m_BanList.BanIP(pszIP, uiMilliseconds);
	return true;
}

static bool FunctionServerUnbanIP(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	const GChar* pszIP = pState->CheckString(0);
	if (!pszIP)
		return false;
	pState->ReturnBoolean(pServer->m_BanList.Remove(pszIP));
	return true;
}

static bool FunctionServerUnbanAllIPs(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	pServer->m_BanList.Clear();
	return true;
}

static bool FunctionServerIsIPBanned(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	const GChar* pszIP = pState->CheckString(0);
	if (!pszIP)
		return false;
	pState->ReturnBoolean(pServer->m_BanList.IsBanned(pszIP));
	return true;
}

static bool FunctionServerShutdown(IScriptState* pState, int argc, void* pUser)
{
	CServer* pServer = (CServer*)pUser;
	_glogprintf(_gstr("Script requested server shutdown!"));
	pServer->m_bExit = true;
	pServer->m_ExitSignal.Signal();
	return true;
}

void CServer::RegisterFunctions(CScripting* pScripting)
{
	pScripting->m_Global.RegisterFunction(_gstr("httpGet"), _gstr("sscc"), FunctionHttpGet, this);
	pScripting->m_Global.RegisterFunction(_gstr("message"), _gstr("s|i"), FunctionMessage, this);
	pScripting->m_Global.RegisterFunction(_gstr("messageClient"), _gstr("sx|i"), FunctionMessageClient, this);
	pScripting->m_Global.RegisterFunction(_gstr("messageAllExcept"), _gstr("sx|i"), FunctionMessageAllExcept, this);

	{
		Galactic3D::ReflectedNamespace* pServerNS = pScripting->m_Global.AddNamespace(_gstr("server"));

		pServerNS->AddProperty(this, _gstr("game"), ARGUMENT_INTEGER, FunctionGetServerGame);
		pServerNS->AddProperty(this, _gstr("name"), ARGUMENT_STRING, FunctionGetServerName, FunctionSetServerName);
		pServerNS->AddProperty(this, _gstr("gameMode"), ARGUMENT_STRING, FunctionGetGameMode, FunctionSetGameMode);
		pServerNS->AddProperty(this, _gstr("port"), ARGUMENT_INTEGER, FunctionGetServerPort);
		pServerNS->AddProperty(this, _gstr("maxClients"), ARGUMENT_INTEGER, FunctionGetServerMaxClients);

		pServerNS->RegisterFunction(_gstr("setRule"), _gstr("ss"), FunctionServerSetRule, this);
		pServerNS->RegisterFunction(_gstr("getRule"), _gstr("s"), FunctionServerGetRule, this);

		pServerNS->RegisterFunction(_gstr("banIP"), _gstr("s|i"), FunctionServerBanIP, this);
		pServerNS->RegisterFunction(_gstr("unbanIP"), _gstr("s"), FunctionServerUnbanIP, this);
		pServerNS->RegisterFunction(_gstr("unbanAllIPs"), _gstr(""), FunctionServerUnbanAllIPs, this);
		pServerNS->RegisterFunction(_gstr("isIPBanned"), _gstr("s"), FunctionServerIsIPBanned, this);

		pServerNS->RegisterFunction(_gstr("shutdown"), _gstr(""), FunctionServerShutdown, this);
	}
}

bool CServer::OnPlayerConnect(const Peer_t Peer)
{
	CIPAddress IP;
	GetPeerIP(Peer, IP);
	GChar szHost[256];
	IP.ToString(szHost, ARRAY_COUNT(szHost), false);
	_glogprintf(_gstr("CONNECT: %s is attempting to connect"), szHost);
	{
		CArguments Args;
		Args.AddString(szHost);
		bool bPreventDefault;
		if (m_pOnPlayerConnectEventType->Trigger(Args, bPreventDefault) && bPreventDefault)
			return false;
	}
	return true;
}

void CServer::OnPlayerDisconnect(const Peer_t Peer, unsigned int uiReason)
{
	if (uiReason == DISCONNECT_NICKNAMEINUSE)
	{
		return;
	}

	//_glogprintf(_gstr("DISCONNECT: %s:%u disconecting."), Host.CString(), pPeer->address.port);
	CNetMachine* pPlayerInfo = m_NetMachines.GetMachineFromPeer(Peer);
	if (pPlayerInfo != NULL)
	{
		{
			CArguments Args(2);
			Args.AddObject(pPlayerInfo);
			Args.AddNumber(uiReason);
			m_pOnPlayerQuitEventType->Trigger(Args);
		}
		for (size_t i = 0; i < m_pManager->m_Objects.GetSize(); i++)
		{
			if (m_pManager->m_Objects.IsUsedAt(i))
			{
				CNetObject* pElement = m_pManager->m_Objects.GetAt(i);

				if (pElement != nullptr)
				{
					if (pElement->GetSyncer() == pPlayerInfo)
					{
						pElement->SetSyncer(nullptr, false);
					}

					pElement->SetCreatedFor(pPlayerInfo, false);
					pElement->ResetMachine(pPlayerInfo);
					m_pManager->PossiblyDeleteObject(pElement);
				}
			}
		}
		pPlayerInfo->DespawnPlayer();
		//tTRPacketDeletePlayer Packet;
		//Packet.m_iPlayerIndex = pPlayerInfo-m_rgPlayers;
		//SendPacket(&Packet,sizeof(Packet));
		//ChatPrintf("%s left the game",pPlayerInfo->m_szName);
		m_CurrentClients--;
		m_Announcer.PlayerRemove(pPlayerInfo);
		auto nPlayerIndex = pPlayerInfo->m_nIndex;
		pPlayerInfo->m_nIndex = -1;
		m_NetMachines.m_rgpMachines[nPlayerIndex].SetNull();
		{
			Packet Packet(PACKET_REMOVEMACHINE);

			Packet.Write<Uint16>(1);
			Packet.Write<Uint32>(nPlayerIndex);
			SendEveryonePacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_NETMACHINES);
		}
	}
	//for (unsigned int i=0; i<ARRAY_COUNT(m_rgPlayers); i++)
	//{
	//	if (m_rgPlayers[i].m_bUsed && m_rgPlayers[i].m_pPeer == pPeer)
	//	{
	//		_glogprintf(_gstr("DISCONNECT: %s:%u reset peer."), Host.CString(), pPeer->address.port);
	//		m_rgPlayers[i].Reset();
	//		break;
	//	}
	//}
}

bool CServer::IsVersionAllowed(uint32_t Major, uint32_t Minor, uint32_t Patch, uint32_t Build)
{
	if (Major > m_uiMinMajorVersion)
		return true;
	if (Major == m_uiMinMajorVersion)
	{
		if (Minor > m_uiMinMinorVersion)
			return true;
		if (Minor == m_uiMinMinorVersion)
		{
			if (Patch > m_uiMinPatchVersion)
				return true;
			if (Patch == m_uiMinPatchVersion)
			{
				if (Build >= m_uiMinBuildVersion)
					return true;
			}
		}
	}
	return false;
}

void CServer::ProcessPacket(const tPeerInfo& Peer, unsigned int PacketID, Stream* pStream)
{
	CBinaryReader Reader(pStream);

	CMafiaClient* pClient = static_cast<CMafiaClient*>(m_NetMachines.GetMachineFromPeer(Peer.m_Peer));

	{
		CArguments Args(2);
		Args.AddNumber(PacketID);
		Args.AddObject(pClient);

		bool bPreventDefault = false;
		m_pManager->m_pOnReceivePacketEventType->Trigger(Args, bPreventDefault);
		if (bPreventDefault)
			return;
	}

	if (PacketID == PACKET_INITIAL)
	{
		// Prevent first packet unless first packet if first packet already received.
		if (pClient != nullptr)
		{
			// TODO: Disconnect. Because it is likely that trailing data was sent with the PACKET_INITIAL opcode.
			return;
		}
	}
	else
	{
		// Prevent packets unless player actually passed the first packet checks.
		if (pClient == nullptr)
		{
			return;
		}
	}

	auto pMafiaManager = static_cast<CMafiaServerManager*>(m_pManager);

	switch (PacketID)
	{
		case PACKET_INITIAL:
			{
				GChar szHost[MAX_IPSTRING] = { 0 };
				CIPAddress IPAddress;
				GetPeerIP(Peer.m_Peer, IPAddress);
				if (!IPAddress.ToString(szHost, ARRAY_COUNT(szHost), false))
				{
					_glogwarnprintf(_gstr("CONNECT: %s failed connection [IP FAIL]"), szHost);
					DisconnectPeer(Peer.m_Peer, DISCONNECT_FAILED);
					return;
				}
				if (m_BanList.IsBanned(szHost))
				{
					_glogwarnprintf(_gstr("CONNECT: %s revoked connection [BANNED]"), szHost);
					DisconnectPeer(Peer.m_Peer, DISCONNECT_BANNED);
					return;
				}
				uint32_t uiNetVersion = 0;
				Reader.ReadUInt32(&uiNetVersion, 1);
				if (uiNetVersion != m_uiNetVersion)
				{
					_glogwarnprintf(_gstr("CONNECT: %s (%d) revoked connection [UNSUPPORTED CLIENT]"), szHost, uiNetVersion);
					DisconnectPeer(Peer.m_Peer, DISCONNECT_UNSUPPORTEDCLIENT);
					return;
				}

				uint32_t uiMajorVersion;
				uint32_t uiMinorVersion;
				uint32_t uiPatchVersion;
				uint32_t uiBuildVersion;
				Reader.ReadUInt32(&uiMajorVersion, 1);
				Reader.ReadUInt32(&uiMinorVersion, 1);
				Reader.ReadUInt32(&uiPatchVersion, 1);
				Reader.ReadUInt32(&uiBuildVersion, 1);

				if (!IsVersionAllowed(uiMajorVersion, uiMinorVersion, uiPatchVersion, uiBuildVersion))
				{
					_glogwarnprintf(_gstr("CONNECT: %s revoked connection [UNSUPPORTED CLIENT]"), szHost);
					DisconnectPeer(Peer.m_Peer, DISCONNECT_UNSUPPORTEDCLIENT);
					return;
				}

				{
					size_t PasswordLength;
					GChar* pszPassword = Reader.ReadString(&PasswordLength);
					if (pszPassword == nullptr)
					{
						_glogwarnprintf(_gstr("CONNECT: %s revoked connection [WRONG PASSWORD]"), szHost);
						DisconnectPeer(Peer.m_Peer, DISCONNECT_WRONGPASSWORD);
						return;
					}
					bool bWrongPassword = m_Password.HasPassword() && !m_Password.Verify(pszPassword);
					GFree(pszPassword);
					if (bWrongPassword)
					{
						_glogwarnprintf(_gstr("CONNECT: %s revoked connection [WRONG PASSWORD]"), szHost);
						DisconnectPeer(Peer.m_Peer, DISCONNECT_WRONGPASSWORD);
						return;
					}
				}
				unsigned short usNicknameLength = 0;
				Reader.ReadUInt16(&usNicknameLength, 1);
				GChar szName[NETGAME_MAX_NAME_BUFFER] = { 0 };
				if (usNicknameLength <= 0 || usNicknameLength >= NETGAME_MAX_NAME_BUFFER || usNicknameLength >= NETGAME_MAX_NAME)
				{
					_glogwarnprintf(_gstr("CONNECT: %s revoked connection [INVALID NICKNAME]"), szHost);
					DisconnectPeer(Peer.m_Peer, DISCONNECT_INVALIDNICKNAME);
					return;
				}
				{
					char szName2[NETGAME_MAX_NAME_BUFFER] = { 0 };
					pStream->Read(szName2, usNicknameLength);
					szName2[usNicknameLength] = '\0';
					CString Name(false, szName2, usNicknameLength);
					_gstrlcpy(szName, Name, ARRAY_COUNT(szName));
					if (_gstrlen(szName) == 0)
					{
						_glogwarnprintf(_gstr("CONNECT: %s revoked connection [INVALID NICKNAME]"), szHost);
						DisconnectPeer(Peer.m_Peer, DISCONNECT_INVALIDNICKNAME);
						return;
					}
				}
				uint8_t ucGame;
				Reader.ReadUInt8(&ucGame, 1);
				if (std::find(m_AllowedGameIds.begin(), m_AllowedGameIds.end(), ucGame) == m_AllowedGameIds.end())
				{
					_glogwarnprintf(_gstr("CONNECT: %s revoked connection [WRONG GAME]"), szName);
					DisconnectPeer(Peer.m_Peer, DISCONNECT_UNSUPPORTEDENGINE);
					return;
				}
				uint8_t ucGameVersion;
				Reader.ReadUInt8(&ucGameVersion, 1);
				if (IsNameInUse(szName))
				{
					_glogwarnprintf(_gstr("CONNECT: %s revoked connection [NICKNAME IN USE]"), szName);
					DisconnectPeer(Peer.m_Peer, DISCONNECT_NICKNAMEINUSE);
					return;
				}
				if (m_CurrentClients >= m_MaxClients)
				{
					_glogwarnprintf(_gstr("CONNECT: %s revoked connection [SERVER FULL]"), szName);
					DisconnectPeer(Peer.m_Peer, DISCONNECT_FULL);
					return;
				}
				Strong<CNetMachine> pNetMachine;
				for (size_t i = 0; i < MAX_MACHINES; i++)
				{
					if (m_NetMachines.m_rgpMachines[i] == nullptr)
					{
						pNetMachine = Strong<CNetMachine>::New(new CMafiaClient(m_pManager));
						m_NetMachines.m_rgpMachines[i] = pNetMachine;
						pNetMachine->m_Peer = Peer.m_Peer;
						const GChar* pszGame = m_pManager->m_Games.GetGameName(ucGame);
						pNetMachine->m_Game.assign(pszGame);
						pNetMachine->m_GameId = ucGame;
						//pNetMachine->m_uiVersion = uiVersion;
						pNetMachine->m_ucGameVersion = ucGameVersion;
						pNetMachine->m_nIndex = (uint32_t)i;
						pNetMachine->m_GlobalIdentifier = Peer.m_GlobalPeer;
						pNetMachine->m_IPAddress = IPAddress;
						pNetMachine->SetName(szName);
						m_CurrentClients++;
						m_Announcer.PlayerAdd(pNetMachine);
						break;
					}
				}
				if (pNetMachine == nullptr)
				{
					_glogwarnprintf(_gstr("CONNECT: %s revoked connection [SERVER FULL]"), szName);
					DisconnectPeer(Peer.m_Peer, DISCONNECT_FULL);
					return;
				}
				else
				{
					{
						CArguments Args(1);
						Args.AddObject(pNetMachine);
						m_pOnPlayerJoinEventType->Trigger(Args);
					}
					OnPlayerJoin(pNetMachine);
					SendCVars(pNetMachine);
					{
						Packet Packet(PACKET_RESPONSE);

						CBinaryWriter Writer(&Packet);
						Writer.WriteInt32(pNetMachine->m_nIndex);
						Writer.WriteUInt16(m_usHTTPPort);
						Writer.WriteString(m_HTTPUrl.c_str(), m_HTTPUrl.length());
						Writer.WriteUInt16(m_usSyncInterval);
						Uint8 ucFlags = 0;
#if 0
						if (m_pNetSystem->m_bCompressPackets)
							ucFlags |= 1;
#endif
						Writer.WriteUInt8(ucFlags);
#if 0
						if (m_pNetSystem->m_bCompressPackets)
							Writer.WriteInt32((int32_t)m_pNetSystem->m_CompressionLevel);
#endif
						Writer.WriteUInt8((uint8_t)m_SyncMethod);

						pNetMachine->SendPacket(&Packet);
					}
				}

				SendMapName(pNetMachine);
			}
			break;
		case PACKET_JOIN:
			{
				if (pClient != nullptr)
				{
					bool bWasJoined = pClient->m_bJoined;
					pClient->m_bJoined = true;

					m_ResourceMgr.UpdateAllResource(this, Peer.m_Peer);

					//_glogprintf(_gstr("Sending gamemode and servername"));
					SendGameMode(pClient);
					SendServerName(pClient);

					//_glogprintf(_gstr("Calling CServer OnPlayerJoined for %i"), pClient->m_nIndex);
					OnPlayerJoined(pClient);

					//_glogprintf(_gstr("Sending PACKET_JOINED to %i"), pClient->m_nIndex);
					// moved OnPlayerJoined here
					{
						CArguments Args(1);
						//Args.AddInt32(iLocalIndex);
						Args.AddObject(pClient);
						m_pOnPlayerJoinedEventType->Trigger(Args);
					}

					if (!bWasJoined)
					{
						// Inform everyone else we joined
						// Calculate PeersSize also while we are looping here...
						size_t PeerCount = 0;
						{
							for (size_t i = 0; i < MAX_MACHINES; i++)
							{
								if (m_NetMachines.m_rgpMachines[i] != nullptr && m_NetMachines.m_rgpMachines[i]->m_bJoined)
								{
									PeerCount++;

									// Don't tell ourself we joined
									if (m_NetMachines.m_rgpMachines[i] != pClient)
									{
										Packet Packet(PACKET_ADDMACHINE);

										Packet.Write<Uint16>(1);
										Packet.Write<Uint32>(pClient->m_nIndex);
										if (pClient->Write(&Packet, m_NetMachines.m_rgpMachines[i]))
											m_NetMachines.m_rgpMachines[i]->SendPacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_NETMACHINES);

										//_glogprintf(_gstr("Sending machine %i to client %i"), i, pClient->m_nIndex);
									}
								}
							}
						}

						// Inform this player who is joined
						{
							Packet Packet(PACKET_EXISTINGMACHINES);
							Packet.Write<Uint16>((Uint16)PeerCount);
							Packet.Write<Uint16>((Uint16)m_MaxClients);
							for (size_t i = 0; i < MAX_MACHINES; i++)
							{
								if (m_NetMachines.m_rgpMachines[i] != nullptr && m_NetMachines.m_rgpMachines[i]->m_bJoined)
								{
									Packet.Write<Uint32>(m_NetMachines.m_rgpMachines[i]->m_nIndex);
									m_NetMachines.m_rgpMachines[i]->Write(&Packet, pClient);
								}
							}
							//_glogprintf(_gstr("Sending PACKET_EXISTINGMACHINES to client %i"), pClient->m_nIndex);
							pClient->SendPacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_NETMACHINES);
						}
					}

					//_glogprintf(_gstr("Sending player to %i"), pClient->m_nIndex);
					pClient->SendPlayer();

					//_glogprintf(_gstr("Sending PACKET_JOINED to %i"), pClient->m_nIndex);
					Packet Packet(PACKET_JOINED);
					pClient->SendPacket(&Packet);
				}
			}
			break;
		case PACKET_PLAYERSYNC:
			{
				unsigned short usCount = 0;
				Reader.ReadUInt16(&usCount, 1);

				for (size_t i = 0; i < usCount; i++)
				{
					uint16_t usSize;
					Reader.ReadUInt16(&usSize, 1);
					int32_t Type;
					Reader.ReadInt32(&Type, 1);
					int32_t nId;
					Reader.ReadInt32(&nId, 1);
					if(usSize >= (sizeof(Type) + sizeof(nId)))
					{
						usSize -= sizeof(Type) + sizeof(nId);
					}
					else
					{
						// Malformed packet.
						// TODO: Flush all of packet PACKET_PLAYERSYNC as packet size is known.
						return;
					}

					CNetObject* pServerElement = m_pManager->FromId(nId);
					if (pServerElement != nullptr)
					{
						// Element is now dirty
						//
						pServerElement->SetDirty(true);

						auto Pos = (size_t)pStream->Tell();
						pServerElement->ReadSyncPacket(pStream);
						size_t Read = ((size_t)pStream->Tell() - Pos);
						assert(Read == usSize); // ReadSyncPacket didn't read all the data
						pStream->Seek(Pos + usSize, SEEK_SET);
					}
					else
						pStream->Seek(usSize, SEEK_CUR);
				}

				if (m_SyncMethod == SYNCMETHOD_REPLAY)
					SendSync(pClient);
			}
			break;
		case PACKET_NETWORKEVENT:
			{
				size_t Length;
				const GChar* pszName = Reader.ReadString(&Length);
				if (pszName != nullptr)
				{
					CArguments Args;
					Args.m_pNetObjectMgr = m_pManager;
					if (Args.Read(pStream))
					{
						if (!m_ResourceMgr.m_pNetworkHandlers->Trigger(pClient, pszName, Args))
						{
							_glogwarnprintf(_gstr("Network event %s not bound serverside"), pszName);
						}
					}
					else
					{
						_glogwarnprintf(_gstr("Failed to read network arguments for network event %s."), pszName);
					}
					GFree(pszName);
				}
			}
			break;
		case PACKET_CHAT:
			{
				unsigned int MessageLength;
				Reader.ReadUInt32(&MessageLength, 1);

				if (MessageLength == 0 || MessageLength > MAX_CHAT_MESSAGE_LENGTH)
				{
					// TODO: Flush this packet.
					return;
				}

				char* pszMessage = new char[MessageLength + 1];
				if (!pszMessage)
				{
					// TODO: Flush this packet.
					return;
				}
				pStream->Read(pszMessage, MessageLength);
				pszMessage[MessageLength] = '\0';

				if (strlen(pszMessage) == 0)
				{
					// Blank chat message, despite MessageLength being more than zero.
					return;
				}

				CString Message(false, pszMessage, MessageLength);
				UserChat(pClient, Message, Message.GetLength());

				delete[] pszMessage;
			}
			break;
		case PACKET_COMMAND:
			{
				unsigned int CommandLength;
				Reader.ReadUInt32(&CommandLength, 1);

				if (CommandLength == 0 || CommandLength > MAX_COMMAND_LENGTH)
				{
					// TODO: Flush this packet.
					return;
				}

				char* pszCommand = new char[CommandLength + 1];
				if (!pszCommand)
				{
					// TODO: Flush this packet.
					return;
				}
				pStream->Read(pszCommand, CommandLength);
				pszCommand[CommandLength] = '\0';

				unsigned int MessageLength;
				Reader.ReadUInt32(&MessageLength, 1);
				if (MessageLength > MAX_COMMAND_MESSAGE_LENGTH)
				{
					delete[] pszCommand;
					// TODO: Flush this packet.
					return;
				}

				char* pszMessage = new char[MessageLength + 1];
				if (!pszMessage)
				{
					delete[] pszCommand;
					// TODO: Flush this packet.
					return;
				}
				pStream->Read(pszMessage, MessageLength);
				pszMessage[MessageLength] = '\0';

				{
					CArguments Args(3);
					if (pClient != nullptr)
						Args.AddObject(pClient);
					else
						Args.AddNull();
					CString Command(false, pszCommand, CommandLength);
					Args.AddString(Command, Command.GetLength());
					CString Message(false, pszMessage, MessageLength);
					Args.AddString(Message, Message.GetLength());
					bool bPreventDefault = false;
					m_pOnPlayerCommandEventType->Trigger(Args, bPreventDefault);
					if (!bPreventDefault)
						m_ResourceMgr.m_pCommandHandlers->Trigger(Command, Message, pClient, pClient->m_bAdministrator);
				}

				/*{
					CEvent* pEvent = new CEvent(m_pOnPlayerCommandEventType);
					pClient->Push(m_ResourceMgr.m_Scripting.m_pState);
					lua_pushlstring(m_ResourceMgr.m_Scripting.m_pState,pszCommand,CommandLength);
					lua_pushlstring(m_ResourceMgr.m_Scripting.m_pState,pszMessage,MessageLength);
					pEvent->Trigger(m_ResourceMgr.m_Scripting.m_pState,3);
					bool bPreventDefault = pEvent->m_bPreventDefault;
					lua_pop(m_ResourceMgr.m_Scripting.m_pState,3);
					pEvent->Release();

					if (!bPreventDefault)
					{
						//String Message;
						//Message = pClient->m_szName;
						//Message += ": ";
						//Message += pszMessage;
						//SendChat(Message.CString(),Message.GetLength(),1);
					}
				}*/

				delete[] pszCommand;
				delete[] pszMessage;
			}
			break;
		case PACKET_KICK:
			{
				unsigned int uiReason = DISCONNECT_GRACEFUL;
				Reader.ReadUInt32(&uiReason, 1);

				OnPlayerDisconnect(Peer.m_Peer, uiReason);

				DisconnectPeer(Peer.m_Peer, uiReason);
			}
			break;
		case PACKET_KEYEVENT:
			{
				uint8_t ucFlags;
				Reader.ReadUInt8(&ucFlags, 1);

				uint32_t ScanCode;
				Reader.ReadUInt32(&ScanCode, 1);

				uint32_t KeyCode;
				Reader.ReadUInt32(&KeyCode, 1);

				uint16_t Mod;
				Reader.ReadUInt16(&Mod, 1);

				int32_t Repeat;
				Reader.ReadInt32(&Repeat, 1);

				SDL_Keysym Key = {};
				Key.scancode = (SDL_Scancode)ScanCode;
				Key.sym = (SDL_Keycode)KeyCode;
				Key.mod = Mod;
				m_ResourceMgr.m_pKeyBinds->Trigger(pClient, Key, Repeat, (ucFlags & 1) != 0);
			}
			break;
		case PACKET_REQUESTSYNCER:
			{
				int32_t nId;
				if (!Reader.ReadInt32(&nId, 1))
					return;

				CNetObject* pElement = m_pManager->FromId(nId);
				if (pElement != nullptr && pElement->CanBeSyncer(pClient))
				{
					pElement->SetSyncer(pClient);
				}
			}
			break;
		case MAFIAPACKET_HUMAN_SHOOT:
			{
				int32_t nId;
				if (!Reader.ReadInt32(&nId, 1))
					return;

				CVector3D vecShotPosition;
				Reader.ReadSingle((float*)&vecShotPosition, 3);

				CNetObject* pPed = m_pManager->FromId(nId);
				if (pPed != nullptr && pPed->GetSyncer() == pClient)
				{
					{
						Packet Packet(MAFIAPACKET_HUMAN_SHOOT);
						Packet.Write<int32_t>(pPed->GetId());
						Packet.Write<CVector3D>(vecShotPosition);
						m_pManager->SendPacketExcluding(&Packet, pClient);
					}
				}
			}
			break;
		case MAFIAPACKET_HUMAN_DROPWEAP:
			{
				int32_t nId;
				if (!Reader.ReadInt32(&nId, 1))
					return;

				CNetObject* pPed = m_pManager->FromId(nId);
				if (pPed != nullptr && pPed->GetSyncer() == pClient)
				{
					{
						Packet Packet(MAFIAPACKET_HUMAN_DROPWEAP);
						Packet.Write<int32_t>(pPed->GetId());
						m_pManager->SendPacketExcluding(&Packet, pClient);
					}
				}
			}
			break;
		case MAFIAPACKET_HUMAN_CHANGEWEAP:
			{
				int32_t nId;
				if (!Reader.ReadInt32(&nId, 1))
					return;

				int32_t nWeapon;
				if (!Reader.ReadInt32(&nWeapon, 1))
					return;

				CNetObject* pPed = m_pManager->FromId(nId);
				if (pPed != nullptr && pPed->GetSyncer() == pClient)
				{
					CServerHuman *pServerHuman = (CServerHuman*)pPed;

					pServerHuman->m_WeaponId = (int16_t)nWeapon;

					{
						Packet Packet(MAFIAPACKET_HUMAN_CHANGEWEAP);
						Packet.Write<int32_t>(pPed->GetId());
						Packet.Write<int32_t>(nWeapon);
						m_pManager->SendPacketExcluding(&Packet, pClient);
					}
				}
			}
			break;
		case MAFIAPACKET_HUMAN_DIE:
			{
				int32_t nTargetId;
				if (!Reader.ReadInt32(&nTargetId, 1))
					return;

				int32_t nAttackerId;
				if (!Reader.ReadInt32(&nAttackerId, 1))
					return;

				CNetObject* pTargetPed = m_pManager->FromId(nTargetId);
				CNetObject* pAttackerPed = m_pManager->FromId(nAttackerId);
				if (pTargetPed != nullptr && pTargetPed->GetSyncer() == pClient)
				{
					{
						Packet Packet(MAFIAPACKET_HUMAN_DIE);
						Packet.Write<int32_t>(pTargetPed->GetId());
						if(pTargetPed != nullptr) {
							Packet.Write<int32_t>(pAttackerPed->GetId());
						} else {
							Packet.Write<int32_t>(INVALID_NETWORK_ID);
						}
						m_pManager->SendPacketExcluding(&Packet, pClient);
					}
				}
			}
			break;

		case MAFIAPACKET_HUMAN_ENTERINGVEHICLE:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			int32_t nVehicleId;
			Reader.ReadInt32(&nVehicleId, 1);

			int8_t nSeat;
			Reader.ReadInt8(&nSeat, 1);

			int32_t nAction;
			Reader.ReadInt32(&nAction, 1);

			int32_t nUnknown;
			Reader.ReadInt32(&nUnknown, 1);

			CNetObject* pPed = m_pManager->FromId(nPedId);
			CNetObject* pVehicle = m_pManager->FromId(nVehicleId);

			if (pPed == nullptr
			|| pVehicle == nullptr
			|| pClient != pPed->GetSyncer()
			|| nSeat < 0
			|| nSeat > 20)
			{
				break;
			}

			{
				Packet Packet(MAFIAPACKET_HUMAN_ENTERINGVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				Packet.Write<int32_t>(nAction);
				Packet.Write<int32_t>(nUnknown);
				m_pManager->SendPacketExcluding(&Packet, pClient);
			}

			if (nSeat == 0 && pVehicle->CanBeSyncer(pClient))
			{
				_glogprintf(_gstr("Setting vehicle %d syncer to %d"), pVehicle->GetId(), pClient->m_nIndex);
				pVehicle->SetSyncer(pClient, true);
			}
		}
		break;

		case MAFIAPACKET_HUMAN_EXITINGVEHICLE:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			int32_t nVehicleId;
			Reader.ReadInt32(&nVehicleId, 1);

			int8_t nSeat;
			Reader.ReadInt8(&nSeat, 1);

			int32_t nAction;
			Reader.ReadInt32(&nAction, 1);

			int32_t nUnknown;
			Reader.ReadInt32(&nUnknown, 1);

			CNetObject* pPed = m_pManager->FromId(nPedId);
			CNetObject* pVehicle = m_pManager->FromId(nVehicleId);

			if (pPed == nullptr
				|| pVehicle == nullptr
				|| pClient != pPed->GetSyncer()
				|| nSeat < 0
				|| nSeat > 20)
			{
				break;
			}

			{
				Packet Packet(MAFIAPACKET_HUMAN_EXITINGVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				Packet.Write<int32_t>(nAction);
				Packet.Write<int32_t>(nUnknown);
				m_pManager->SendPacketExcluding(&Packet, pClient);
			}
		}
		break;

		case MAFIAPACKET_HUMAN_EXITEDVEHICLE:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			int32_t nVehicleId;
			Reader.ReadInt32(&nVehicleId, 1);

			int8_t nSeat;
			Reader.ReadInt8(&nSeat, 1);

			int32_t nAction;
			Reader.ReadInt32(&nAction, 1);

			int32_t nUnknown;
			Reader.ReadInt32(&nUnknown, 1);

			CNetObject* pPed = m_pManager->FromId(nPedId);
			CNetObject* pVehicle = m_pManager->FromId(nVehicleId);

			if (pPed == nullptr
				|| pVehicle == nullptr
				|| pClient != pPed->GetSyncer()
				|| nSeat < 0
				|| nSeat > 20)
			{
				break;
			}

			{
				Packet Packet(MAFIAPACKET_HUMAN_EXITEDVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				Packet.Write<int32_t>(nAction);
				Packet.Write<int32_t>(nUnknown);
				m_pManager->SendPacketExcluding(&Packet, pClient);
			}
		}
		break;

		case MAFIAPACKET_HUMAN_ENTEREDVEHICLE:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			int32_t nVehicleId;
			Reader.ReadInt32(&nVehicleId, 1);

			int8_t nSeat;
			Reader.ReadInt8(&nSeat, 1);

			int32_t nAction;
			Reader.ReadInt32(&nAction, 1);

			int32_t nUnknown;
			Reader.ReadInt32(&nUnknown, 1);

			CNetObject* pPed = m_pManager->FromId(nPedId);
			CNetObject* pVehicle = m_pManager->FromId(nVehicleId);

			if (pPed == nullptr
				|| pVehicle == nullptr
				|| pClient != pPed->GetSyncer()
				|| nSeat < 0
				|| nSeat > 20)
			{
				break;
			}

			{
				Packet Packet(MAFIAPACKET_HUMAN_ENTEREDVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				Packet.Write<int32_t>(nAction);
				Packet.Write<int32_t>(nUnknown);
				m_pManager->SendPacketExcluding(&Packet, pClient);
			}

			if (nSeat == 0 && pVehicle->CanBeSyncer(pClient))
			{
				_glogprintf(_gstr("Setting vehicle %d syncer to %d"), pVehicle->GetId(), pClient->m_nIndex);
				pVehicle->SetSyncer(pClient, true);
			}
		}
		break;

		case MAFIAPACKET_HUMAN_JACKVEHICLE:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			int32_t nVehicleId;
			Reader.ReadInt32(&nVehicleId, 1);

			int8_t nSeat;
			Reader.ReadInt8(&nSeat, 1);

			CNetObject* pPed = m_pManager->FromId(nPedId);
			CNetObject* pVehicle = m_pManager->FromId(nVehicleId);

			if (pPed == nullptr
				|| pVehicle == nullptr
				|| pClient != pPed->GetSyncer()
				|| nSeat < 0
				|| nSeat > 20)
			{
				break;
			}

			{
				Packet Packet(MAFIAPACKET_HUMAN_JACKVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				m_pManager->SendPacketExcluding(&Packet, pClient);
			}

			if (nSeat == 0)
			{
				pVehicle->SetSyncer(pClient);
			}
		}
		break;

		case MAFIAPACKET_VEHICLE_CREATE:
		{
			uint64_t nLocalVehicleId = 0;
			Reader.ReadUInt64(&nLocalVehicleId, 1);

			//CNetObject* pServerElement = m_pManager->FromId(nLocalVehicleId);
			//if (pServerElement == nullptr)

			Strong<CServerVehicle> pServerVehicle;

			{
				pServerVehicle = Strong<CServerVehicle>::New(m_pManager->Create(ELEMENT_VEHICLE));

				if (pServerVehicle == nullptr)
					return;

				pServerVehicle->m_pResource = nullptr;
				pServerVehicle->ReadCreatePacket(pStream);

				//if(!m_pManager->RegisterObject(pServerVehicle))
				//	return;

				pServerVehicle->SetCreatedFor(pClient, true);
				pServerVehicle->SetSyncer(pClient, true);
			}

			{
				Packet Packet(MAFIAPACKET_ELEMENT_UPDATE_ID);
				Packet.Write<uint64_t>(nLocalVehicleId);
				Packet.Write<int32_t>(pServerVehicle->GetId());
				pClient->SendPacket(&Packet);
			}
		}
		break;

		case MAFIAPACKET_ELEMENT_REMOVE:
		{
			bool bRemoveByElementId;
			Reader.ReadBoolean(bRemoveByElementId);

			if (bRemoveByElementId)
			{
				uint32_t nServerVehicleId = 0;
				Reader.ReadUInt32(&nServerVehicleId, 1);

				CServerVehicle* pServerVehicle = (CServerVehicle*)m_pManager->FromId(nServerVehicleId);

				if (pServerVehicle != nullptr)
				{
					printf("Destroyed vehicle ID %i\n", pServerVehicle->GetId());

					{
						Packet Packet(MAFIAPACKET_ELEMENT_REMOVE);
						Packet.Write<int32_t>(pServerVehicle->GetId());
						m_pManager->SendPacketExcluding(&Packet, pClient);
					}

					m_pManager->Remove(pServerVehicle);
				}
			}
			else
			{
				uint64_t nServerVehicleGuid = 0;
				Reader.ReadUInt64(&nServerVehicleGuid, 1);
			}
		}
		break;

		default:
			break;
	}
}

void CServer::SetGame(const GChar* pszName)
{
	m_Game = pszName;
	m_GameId = m_pManager->m_Games.GetGameId(pszName);
	m_AllowedGameIds.clear();
	m_AllowedGameIds.push_back(m_GameId);
}

void CServer::AddGame(const GChar* pszName)
{
	int32_t nGameId = m_pManager->m_Games.GetGameId(pszName);
	if (nGameId != -1)
		m_AllowedGameIds.push_back(nGameId);
}

void CServer::SendSync(CNetMachine* pClient)
{
	//m_pManager->CreateElementsAsNeeded(pClient,ELEMENT_ELEMENT);
	//m_pManager->DeleteFarAwayStuff(pClient,ELEMENT_ELEMENT);
	m_pManager->SendSync(pClient, m_SyncPacketPriority, m_SyncPacketFlags, true);
}

bool CServer::ReceiveDatagram(CNetSocket* pNetSocket)
{
	return m_UGP.ReceiveDatagram(pNetSocket);
}

void CServer::ManageElements(CNetMachine* pClient)
{
	if (!pClient->m_bJoined)
		return;

	m_pManager->CreateObjectsAsNeeded(pClient, ELEMENT_VEHICLE); // create vehicles first
	m_pManager->DeleteFarAwayStuff(pClient, ELEMENT_PED);
	m_pManager->DeleteFarAwayStuff(pClient, ELEMENT_VEHICLE);

	m_pManager->CreateObjectsAsNeeded(pClient, NETOBJECT_NETOBJECT);
	m_pManager->DeleteFarAwayStuff(pClient, NETOBJECT_NETOBJECT);
}

void CServer::SendAllSync(void)
{
	if (m_SyncMethod == SYNCMETHOD_INTERVAL)
	{
		for (size_t i = 0; i < MAX_MACHINES; i++)
		{
			if (m_NetMachines.m_rgpMachines[i] != nullptr && m_NetMachines.m_rgpMachines[i]->m_bJoined)
			{
				SendSync(m_NetMachines.m_rgpMachines[i]);
			}
		}
	}

	// Find a new syncer
	for (size_t i=0; i<m_pManager->m_Objects.GetSize(); i++)
	{
		if (m_pManager->m_Objects.IsUsedAt(i))
		{
			CNetObject* pServerElement = m_pManager->m_Objects.GetAt(i);
			if (pServerElement != nullptr && pServerElement->m_Flags.m_bFindSyncer)
			{
				if (pServerElement->GetSyncer() == nullptr && !pServerElement->m_Flags.m_bForcedSyncer)
				{
					for (size_t x = 0; x < MAX_MACHINES; x++)
					{
						auto pNetMachine = m_NetMachines.GetMachine(x);
						if (pNetMachine != nullptr)
						{
							if (pServerElement->IsCreatedFor(pNetMachine))
							{
								pServerElement->SetSyncer(pNetMachine);
								break;
							}
						}
					}
				}
			}
		}
	}
}

void CServer::SendChatExcept(CNetMachine* pClient, const GChar* pszMessage, size_t MessageLength, unsigned int uiColour)
{
	Packet Packet(PACKET_CHAT);
	UTF8String Message(false, pszMessage, MessageLength);
	Packet.Write<Uint32>((Uint32)Message.GetLength());
	Packet.Write<Uint32>(uiColour);
	Packet.Write(Message.CString(), Message.GetLength());
	m_pManager->SendPacketExcluding(&Packet, pClient, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_CHAT);
}

void CServer::SendChat(CNetMachine* pClient, const GChar* pszMessage, size_t MessageLength, unsigned int uiColour)
{
	Packet Packet(PACKET_CHAT);
	UTF8String Message(false, pszMessage, MessageLength);
	Packet.Write<Uint32>((Uint32)Message.GetLength());
	Packet.Write<Uint32>(uiColour);
	Packet.Write(Message.CString(), Message.GetLength());
	if (pClient == nullptr)
		SendEveryonePacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_CHAT);
	else
		pClient->SendPacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_CHAT);
}

void CServer::UserChat(CNetMachine* pClient, const GChar* pszMessage, size_t MessageLength)
{
	CArguments Args(2);
	if (pClient != nullptr)
		Args.AddObject(pClient);
	else
		Args.AddNull();
	Args.AddString(pszMessage,MessageLength);
	bool bPreventDefault = false;
	m_pOnPlayerChatEventType->Trigger(Args,bPreventDefault);

	if (!bPreventDefault)
	{
		GString Message(pClient->GetName(), pClient->GetNameLength());
		Message += _gstr(": ");
		Message += pszMessage;
		_glogprintf(_gstr("%s"), Message.c_str());
		SendChat(nullptr, Message.c_str(), Message.length(), 1);
	}
}

bool CServer::ParseConfig(const CServerConfiguration& Config)
{
	auto pFileLog = m_pContext->GetFileLog();

	_gstrlcpy(m_szLogPath, Config.GetStringValue(_gstr("logpath"), _gstr("")), ARRAY_COUNT(m_szLogPath));
	_gstrlcpy(pFileLog->m_szLogTimeStamp, Config.GetStringValue(_gstr("logtimestamp"), _gstr("%d/%m/%Y - %X")), ARRAY_COUNT(pFileLog->m_szLogTimeStamp));

	if (m_szLogPath[0] != '\0')
	{
		GChar szLogFile[64];
		_gsnprintf(szLogFile, ARRAY_COUNT(szLogFile), _gstr("%s/ServerLog.log"), m_szLogPath);

		auto pStream = m_pContext->GetFileSystem()->Open(szLogFile, true);
		if (pStream == nullptr)
			pStream = m_pContext->GetFileSystem()->Create(szLogFile);
		if (pStream != nullptr)
		{
			pStream->Seek(0, SEEK_END);

			pFileLog->StartLogging(pStream);
		}
	}

	_gstrlcpy(m_szBindIP, Config.GetStringValue(_gstr("bindip"), _gstr("")), ARRAY_COUNT(m_szBindIP));
	_gstrlcpy(m_szServerName, Config.GetStringValue(_gstr("servername"), _gstr("")), ARRAY_COUNT(m_szServerName));
	_gstrlcpy(m_szLevel, Config.GetStringValue(_gstr("mapname"), _gstr("")), ARRAY_COUNT(m_szLevel));
	_gstrlcpy(m_szGameMode, Config.GetStringValue(_gstr("gamemode"), _gstr("")), ARRAY_COUNT(m_szGameMode));

	_gstrlcpy(m_szServerListing, Config.GetStringValue(_gstr("serverlistingurl"), _gstr("")), ARRAY_COUNT(m_szServerListing));
	if (m_szServerListing[0] == '\0')
		_gstrcpy_s(m_szServerListing, ARRAY_COUNT(m_szServerListing), _gstr("serverlisting.gtaconnected.com"));

#ifdef _DEBUG
	m_uiFakeNetVersion = Config.GetInt32Value(_gstr("fakenetversion"), 0);
#endif

	m_bServerBrowser = Config.GetBoolValue(_gstr("serverbrowser"), false);

	m_MaxClients = (size_t)Config.GetInt32Value(_gstr("maxplayers"), 32);

	if (m_MaxClients < 1)
	{
		_glogerrorprintf(_gstr("The max players should at least be 1!"));
		return false;
	}

	if (m_MaxClients > 255)
	{
		_glogerrorprintf(_gstr("The max players should not be set to anything more than 255!"));
		return false;
	}

	m_bServerQuery = Config.GetBoolValue(_gstr("serverquery"), true);
	m_usPort = (uint16_t)Config.GetInt32Value(_gstr("port"), 22000);
	m_usHTTPPort = (uint16_t)Config.GetInt32Value(_gstr("httpport"), 22000);
	m_HTTPUrl = Config.GetStringValue(_gstr("httpurl"), _gstr(""));
	m_usRConPort = (uint16_t)Config.GetInt32Value(_gstr("rconport"), 23000);

	m_bHttpServer = Config.GetBoolValue(_gstr("httpserver"), true);
	m_bRCon = Config.GetBoolValue(_gstr("rcon"), false);

	if (m_bRCon && m_usRConPort == m_usHTTPPort)
	{
		_glogerrorprintf(_gstr("The rconport and httpport cannot match!"));
		return false;
	}

	if (m_bRCon)
	{
		const GChar* pszPassword = Config.GetStringValue(_gstr("rconpassword"), nullptr);
		if (pszPassword != nullptr)
		{
			if (!m_RCon.m_Password.SetPassword(pszPassword))
			{
				_glogerrorprintf(_gstr("Failed to set the rcon password!"));
				m_bRCon = false;
			}
		}
	}

	m_usSyncInterval = (uint16_t)Config.GetInt32Value(_gstr("syncinterval"), 30);
	m_iWaitEventTimeout = (int32_t)Config.GetInt32Value(_gstr("waiteventtimeout"), 5);

	const GChar* pszSyncMethod = Config.GetStringValue(_gstr("syncmethod"), _gstr("interval"));
	if (pszSyncMethod != nullptr)
	{
		if (_gstrcasecmp(pszSyncMethod, _gstr("none")) == 0)
			m_SyncMethod = SYNCMETHOD_NONE;
		else if (_gstrcasecmp(pszSyncMethod, _gstr("interval")) == 0)
			m_SyncMethod = SYNCMETHOD_INTERVAL;
		else if (_gstrcasecmp(pszSyncMethod, _gstr("replay")) == 0)
			m_SyncMethod = SYNCMETHOD_REPLAY;
		else
		{
			_glogerrorprintf(_gstr("Invalid syncmethod specified, using interval!"));
			m_SyncMethod = SYNCMETHOD_INTERVAL;
		}
	}
	else
		m_SyncMethod = SYNCMETHOD_INTERVAL;

	const GChar* pszSyncPriority = Config.GetStringValue(_gstr("syncpacketpriority"), _gstr("low"));
	if (pszSyncPriority != nullptr)
	{
		if (_gstrcasecmp(pszSyncPriority, _gstr("immediate")) == 0)
			m_SyncPacketPriority = PACKETPRIORITY_IMMEDIATE;
		else if (_gstrcasecmp(pszSyncPriority, _gstr("high")) == 0)
			m_SyncPacketPriority = PACKETPRIORITY_HIGH;
		else if (_gstrcasecmp(pszSyncPriority, _gstr("medium")) == 0)
			m_SyncPacketPriority = PACKETPRIORITY_MEDIUM;
		else if (_gstrcasecmp(pszSyncPriority, _gstr("low")) == 0)
			m_SyncPacketPriority = PACKETPRIORITY_LOW;
		else
		{
			_glogerrorprintf(_gstr("Invalid syncpriority specified, using low!"));
			m_SyncPacketPriority = PACKETPRIORITY_LOW;
		}
	}

	m_SyncPacketFlags = PACKETFLAGS_NONE;

	if (Config.GetBoolValue(_gstr("syncpacketreliable"), false))
		m_SyncPacketFlags = (ePacketFlags)(m_SyncPacketFlags | PACKETFLAGS_RELIABLE);

	if (Config.GetBoolValue(_gstr("syncpacketunsequenced"), false))
		m_SyncPacketFlags = (ePacketFlags)(m_SyncPacketFlags | PACKETFLAGS_UNSEQUENCED);

#if 0
	m_pNetSystem->m_bCompressPackets = Config.GetBoolValue(_gstr("compresspackets"), false);
	const GChar* pszCompressionLevel = Config.GetStringValue(_gstr("packetcompressionlevel"), nullptr);
	if (pszCompressionLevel != nullptr)
		m_pNetSystem->m_CompressionLevel = _gatoi(pszCompressionLevel);
	else
		m_pNetSystem->m_CompressionLevel = Z_DEFAULT_COMPRESSION;
#endif

	m_bDuplicateNames = Config.GetBoolValue(_gstr("duplicatenames"), false);

	{
		m_uiMinMajorVersion = 1;
		m_uiMinMinorVersion = 0;
		m_uiMinPatchVersion = 0;
		m_uiMinBuildVersion = 0;

		const GChar* pszMinClientVersion = Config.GetStringValue(_gstr("minclientversion"), nullptr);
		if (pszMinClientVersion != nullptr)
		{
			_gsscanf(pszMinClientVersion, _gstr("%d.%d.%d.%d"), &m_uiMinMajorVersion, &m_uiMinMinorVersion, &m_uiMinPatchVersion, &m_uiMinBuildVersion);
		}

		//_glogprintf(_gstr("Minimum client version: %d.%d.%d"), m_uiMinMajorVersion, m_uiMinMinorVersion, m_uiMinPatchVersion);
	}

	m_pManager = new CMafiaServerManager(m_pContext, this);
	m_pManager->m_pNetMachines = &m_NetMachines;

	m_Password.SetPassword(Config.GetStringValue(_gstr("password"), nullptr));

	m_bAllowModdedExecutables = true;

	if (m_bAllowModdedExecutables)
	{
		//_glogerrorprintf(_gstr("************************************************************************"));
		//_glogerrorprintf(_gstr("WARNING: Server is allowing modified executables!!!"));
		//_glogerrorprintf(_gstr("WARNING: This can cause potential security breaches!!!"));
		//_glogerrorprintf(_gstr("WARNING: It is recommended that you disable this when you are done testing!!!"));
		//_glogerrorprintf(_gstr("************************************************************************"));
	}

	const CElementChunk* pModulesElement = Config.m_pConfig->FirstChildElement(_gstr("modules"));
	if (pModulesElement != nullptr)
	{
		int iTotal = 0;
		int iSuccess = 0;
		int iErrors = 0;
		_glogprintf(_gstr("Loading modules..."));
		for (const CElementChunk* pResourceElement = pModulesElement->FirstChildElement(_gstr("module")); pResourceElement != nullptr; pResourceElement = pResourceElement->NextSiblingElement())
		{
			iTotal++;
			const GChar* pszName = pResourceElement->Attribute(_gstr("src"));
			if (pszName == nullptr)
			{
				_glogerrorprintf(_gstr("Error loading module: attribute src not defined!"));
				iErrors++;
			}
			else
			{
				if (m_ResourceMgr.LoadModule(pszName))
				{
					_glogprintf(_gstr("Loaded module %s"), pszName);
					iSuccess++;
				}
				else
				{
					_glogerrorprintf(_gstr("Error loading module %s!"), pszName);
					iErrors++;
				}
			}
		}
		_glogmessageprintf((iErrors > 0) ? LOGTYPE_WARN : LOGTYPE_INFO, _gstr("%i/%i Modules loaded, %i failed"), iSuccess, iTotal, iErrors);
	}

	if (!m_ResourceMgr.DefineAllClasses())
		return false;

	m_ResourceMgr.RefreshResources(false);

	const CElementChunk* pResourcesElement = Config.m_pConfig->FirstChildElement(_gstr("resources"));
	if (pResourcesElement != nullptr)
	{
		for (const CElementChunk* pResourceElement = pResourcesElement->FirstChildElement(_gstr("resource")); pResourceElement != nullptr; pResourceElement = pResourceElement->NextSiblingElement())
		{
			const GChar* pszName = pResourceElement->Attribute(_gstr("src"));
			if (pszName != nullptr)
			{
				auto pResource = m_ResourceMgr.FindResourceByName(pszName);
				if (pResource != nullptr)
					pResourceElement->GetBoolAttribute(_gstr("privileged"), &pResource->m_bPrivileged);
				m_InitialResources.push_back(pszName);
			}
		}
	}

	const CElementChunk* pCVarElement = Config.m_pConfig->FirstChildElement(_gstr("cvar"));
	if (pCVarElement != nullptr)
	{
		for (; pCVarElement != nullptr; pCVarElement = pCVarElement->NextSiblingElement(_gstr("cvar")))
		{
			const GChar* pszName = pCVarElement->Attribute(_gstr("name"));
			const GChar* pszValue = pCVarElement->Attribute(_gstr("value"));
			if (pszName == nullptr)
			{
				_glogerrorprintf(_gstr("CVar in config without name attribute!"));
				continue;
			}
			if (pszValue == nullptr)
			{
				_glogerrorprintf(_gstr("CVar %s in config without value attribute!"), pszName);
				continue;
			}
			if (!m_CVars.Set(pszName, pszValue, true))
			{
				_glogerrorprintf(_gstr("Unknown CVar %s!"), pszName);
				continue;
			}
		}
	}

	const CElementChunk* pRuleElement = Config.m_pConfig->FirstChildElement(_gstr("rule"));
	if (pRuleElement != nullptr)
	{
		for (; pRuleElement != nullptr; pRuleElement = pRuleElement->NextSiblingElement(_gstr("rule")))
		{
			const GChar* pszName = pRuleElement->Attribute(_gstr("name"));
			const GChar* pszValue = pRuleElement->Attribute(_gstr("value"));
			if (pszName == nullptr)
			{
				_glogerrorprintf(_gstr("Rule in config without name attribute!"));
				continue;
			}
			if (pszValue == nullptr)
			{
				_glogerrorprintf(_gstr("Rule %s in config without value attribute!"), pszName);
				continue;
			}
			SetRule(pszName, pszValue);
		}
	}

	SetGame(Config.GetStringValue(_gstr("game"), _gstr("mafia:one")));

	if (m_GameId == GAME_MAFIA_ONE)
	{
		auto pStream = Strong<Stream>::New(m_pContext->GetFileSystem()->Open(_gstr("/MafiaInterface/MafiaOneEnums.h"), false));
		if (pStream != nullptr)
			m_ResourceMgr.m_pDefineHandlers->Process(pStream, _gstr("MafiaOneEnums.h"));
	}

	const CElementChunk* pGamesElement = Config.m_pConfig->FirstChildElement(_gstr("games"));
	if (pGamesElement != nullptr)
	{
	}

	m_fStreamInDistance = Config.GetFloatValue(_gstr("streamindistance"), 100.0f);
	m_fStreamOutDistance = Config.GetFloatValue(_gstr("streamoutdistance"), 200.0f);

	m_fPickupStreamInDistance = Config.GetFloatValue(_gstr("pickupstreamindistance"), 50.0f);
	m_fPickupStreamOutDistance = Config.GetFloatValue(_gstr("pickupstreamoutdistance"), 100.0f);

	m_bNametags = Config.GetBoolValue(_gstr("nametags"), true);
	m_bWeatherSync = Config.GetBoolValue(_gstr("weathersync"), true);

	return true;
}

bool CServer::LoadConfig(const GChar* pszPath)
{
	auto pStream = Strong<Stream>::New(CFileMgr::Open(pszPath, false));
	if (!pStream)
	{
		_glogerrorprintf(_gstr("I can't start because server.xml doesn't exist..."));
		return false;
	}
	CServerConfiguration Config;
	if (!Config.Read(pStream))
	{
		_glogerrorprintf(_gstr("I can't start because server.xml failed to parse..."));
		return false;
	}
	if (!ParseConfig(Config))
		return false;
	return true;
}

bool CServer::StartInitialResources()
{
	int iTotal = 0;
	int iSuccess = 0;
	int iErrors = 0;
	_glogprintf(_gstr("Starting resources..."));
	for (auto Resource : m_InitialResources)
	{
		iTotal++;
		CResource* pResource;
		if ((pResource = m_ResourceMgr.FindResourceByName(Resource.c_str())) != NULL)
		{
			if (pResource->Start())
				iSuccess++;
			else
			{
				_glogerrorprintf(_gstr("Error loading resource %s!"), Resource.c_str());
				iErrors++;
			}
		}
		else
		{
			_glogerrorprintf(_gstr("Error loading resource %s: resource not found!"), Resource.c_str());
			iErrors++;
		}
	}
	_glogmessageprintf((iErrors > 0) ? LOGTYPE_WARN : LOGTYPE_INFO, _gstr("%i/%i Resources loaded, %i failed"), iSuccess, iTotal, iErrors);
	m_InitialResources.clear();
	return iErrors == 0;
}

bool CServer::StartServer(void)
{
	_glogprintf(_gstr("Server is starting..."));

	m_TimeManager.Initialise(60.0,16.0);

	if (!InitAsServer(m_usPort, m_szBindIP))
	{
		_glogerrorprintf(_gstr("Failed to start the server!"));
		return false;
	}

	_glogprintf(_gstr("Server is now listening on port %hu max players %d..."), m_usPort, (unsigned int)m_MaxClients);

	if (m_bRCon)
	{
		if (m_RCon.StartServer(m_usRConPort))
		{
			_glogerrorprintf(_gstr("Failed to start the RCon server!"));
			return false;
		}
	}
	if (m_bHttpServer)
	{
		if (!m_HttpServer.StartServer(m_szBindIP, m_usHTTPPort))
		{
			_glogerrorprintf(_gstr("Failed to start the HTTP server!"));
			return false;
		}
	}

	{
		m_pConsole = Strong<CNetMachine>::New(new CMafiaClient(m_pManager));
		m_pConsole->m_Game = m_Game;
		m_pConsole->m_GameId = m_GameId;
		//m_pConsole->m_uiVersion = m_uiVersionMax;
		m_pConsole->m_ucGameVersion = 0;
		m_pConsole->m_nIndex = 255;
		m_pConsole->m_bAdministrator = true;
		m_pConsole->m_bConsole = true;
		m_pConsole->m_bJoined = true; // Consequences?
		m_pConsole->SetName(_gstr("Console"));
		m_NetMachines.m_nOurMachineId = 255;
		m_NetMachines.m_rgpMachines[255] = m_pConsole;
	}

	StartInitialResources();
	m_pOnServerStartEventType->Trigger();
	return true;
}

void CServer::ShutDown()
{
	if (m_pInputThread != nullptr)
	{
		delete m_pInputThread;
		m_pInputThread = nullptr;
	}

	_glogprintf(_gstr("Server shutting down..."));

	m_InputEvent.Clear();
}

void CServer::StartInputThread()
{
	if (m_pInputThread == nullptr)
	{
		CThreadCB* pThread = new CThreadCB("InputThread", [](Thread* pThread, void* pUser) {
			((CServer*)pUser)->InputThread();
			return 0;
		}, this);
		if (pThread->Start())
			m_pInputThread = pThread;
		else
			delete pThread;
	}
}

bool CServer::StartMasterlist(void)
{
	_glogprintf(_gstr("Connecting to the server listing..."));
	GChar szURL[256];
	_gsnprintf(szURL, ARRAY_COUNT(szURL), _gstr("ws://%s/"), m_szServerListing);
	GChar szExtraHeaders[256];
	_gsnprintf(szExtraHeaders, ARRAY_COUNT(szExtraHeaders), _gstr("User-Agent: %s\r\n"), m_pContext->GetUserAgent());
	if (!m_Announcer.Connect(szURL, _gstr("ws_serverlisting3"), szExtraHeaders))
	{
		_glogerrorprintf(_gstr("Failed connecting to the server listing!"));
		return false;
	}
	return true;
}

bool CServer::OnFrame(void)
{
	CLockable::CLock Lock(&m_Lock, true);

	const FrameTimeInfo* pTime = m_TimeManager.Process();

	m_ResourceMgr.Process(pTime->m_fDeltaTime);
	m_HTTPMgr.Process();
	if (m_bRCon)
		m_RCon.UpdateNetwork();
	if (m_bExit || m_ExitSignal.IsSignalled())
		return false;
	UpdateNetwork();
	if (m_bServerBrowser)
	{
		m_Announcer.Pulse((float)pTime->m_fDeltaTime);
	}
	{
		for (size_t i=0; i<MAX_MACHINES; i++)
		{
			if (m_NetMachines.m_rgpMachines[i] != nullptr && m_NetMachines.m_rgpMachines[i]->m_bJoined)
			{
				ManageElements(m_NetMachines.m_rgpMachines[i]);
			}
		}
	}
	SendAllSync();
	OnProcess(pTime);
	m_UGP.UpdateResponse();
	return true;
}

void CServer::MainLoop(void)
{
	while (OnFrame())
	{
		for (size_t i = 0; i < m_InputEvent.Count(); i++)
		{
			m_ResourceMgr.m_pCommandHandlers->ConsoleCommandOnly((const GChar*)m_InputEvent.GetEvent(i), m_pConsole, true);
		}

		m_InputEvent.Flush();

		OS::Delay(5);
	}
}

void CServer::InputThread()
{
	while (!m_ExitSignal.IsSignalled())
	{
#ifdef _WIN32
		wchar_t szBuffer[1024];
		DWORD dwNumberOfCharsRead;
		if (ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), szBuffer, ARRAY_COUNT(szBuffer) - 1, &dwNumberOfCharsRead, nullptr))
		{
			szBuffer[dwNumberOfCharsRead] = '\0';

			ProcessConsoleInput(CString(false, szBuffer, dwNumberOfCharsRead));
		}
#else
		GChar szInput[256];
		if (_gfgets(szInput, ARRAY_COUNT(szInput), stdin) != nullptr)
			ProcessConsoleInput(szInput);
#endif
	}
}

void CServer::SetGameMode(const GChar* pszGameMode)
{
	_gstrlcpy(m_szGameMode, pszGameMode, ARRAY_COUNT(m_szGameMode));
	m_Announcer.SetGameMode(pszGameMode, _gstrlen(m_szGameMode));

	SendGameMode(nullptr);
}

void CServer::SendGameMode(CNetMachine* pClient)
{
	Packet Packet(PACKET_SETGAMEMODE);
	CBinaryWriter Writer(&Packet);
	Writer.WriteString(m_szGameMode);
	if (pClient == nullptr)
		SendEveryonePacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_INFO);
	else
		pClient->SendPacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_INFO);
}

void CServer::SetServerName(const GChar* pszName)
{
	_gstrlcpy(m_szServerName, pszName, ARRAY_COUNT(m_szServerName));
	m_Announcer.SetName(pszName, _gstrlen(m_szServerName));

	SendGameMode(nullptr);
}

void CServer::SetMapName(const GChar* pszName)
{
	_gstrlcpy(m_szLevel, pszName, ARRAY_COUNT(m_szLevel));

	SendMapName(nullptr);
}

void CServer::SendServerName(CNetMachine* pClient)
{
	Packet Packet(PACKET_SETSERVERNAME);
	CBinaryWriter Writer(&Packet);
	Writer.WriteString(m_szServerName);
	if (pClient == nullptr)
		SendEveryonePacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_INFO);
	else
		pClient->SendPacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_INFO);
}

void CServer::SendMapName(CNetMachine* pClient)
{
	Packet Packet(MAFIAPACKET_CHANGEMAP);
	CBinaryWriter Writer(&Packet);
	Writer.WriteString(m_szLevel);

	if (pClient == nullptr)
		SendEveryonePacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_INFO);
	else
		pClient->SendPacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_INFO);
}

void CServer::SendCVars(CNetMachine* pClient)
{
	Packet Packet(PACKET_SETCLIENTVAR);
	m_CVars.Serialise(&Packet);
	if (pClient == nullptr)
		SendEveryonePacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_INFO);
	else
		pClient->SendPacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_INFO);
}

void CServer::OnProcess(const FrameTimeInfo* pTime)
{
	{
		CArguments Args(1);
		Args.AddNumber(pTime->m_fDeltaTime);
		m_pOnProcessEventType->Trigger(Args);
	}
}

void CServer::OnPlayerJoin(CNetMachine* pClient)
{
}

void CServer::OnPlayerJoined(CNetMachine* pClient)
{
	auto pManager = static_cast<CMafiaServerManager*>(m_pManager);
}

void CServer::ProcessConsoleInput(const GChar* pszCommandString)
{
	GChar* pszCommandString2 = _gstrdup(pszCommandString);
	if (pszCommandString2 == nullptr)
		return;
	m_InputEvent.Push(pszCommandString2);
}

bool CServer::IsNameInUse(const GChar* pszName)
{
	if (m_bDuplicateNames)
		return false;
	for (size_t i = 0; i<MAX_MACHINES; i++)
	{
		if (m_NetMachines.m_rgpMachines[i] != NULL)
		{
			if (_gstrncasecmp(pszName, m_NetMachines.m_rgpMachines[i]->GetName(), m_NetMachines.m_rgpMachines[i]->GetNameLength() + 1) == 0)
			{
				return true;
			}
		}
	}
	return false;
}

void CServer::SetRule(const GChar* pszKey, const GChar* pszValue)
{
	for (auto& Rule : m_Rules)
	{
		if (Rule.m_Key == pszKey)
		{
			Rule.m_Value = pszValue;
			return;
		}
	}

	tRule Rule;
	Rule.m_Key = pszKey;
	Rule.m_Value = pszValue;
	m_Rules.push_back(Rule);
}

const GChar* CServer::GetRule(const GChar* pszKey)
{
	for (const auto& Rule : m_Rules)
	{
		if (Rule.m_Key == pszKey)
			return Rule.m_Value.c_str();
	}
	return nullptr;
}

void CServer::SyncroniseEnterVehicle(CServerHuman* pHuman, CServerVehicle* pVehicle, bool bDriver)
{
}

void CServer::SyncroniseExitVehicle(CServerHuman* pHuman)
{
}