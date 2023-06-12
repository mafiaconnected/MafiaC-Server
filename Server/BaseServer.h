#pragma once

#include <Multiplayer/Multiplayer.h>
#include <Multiplayer/NetRPC.h>
#include "ServerResourceMgr.h"
#include "ServerManager.h"
#include "BanList.h"
#include <Multiplayer/ClientDownloadManager.h> // fix this?
#include "UnleashedGameProtocolHandler.h"
#include <Network/MongooseSystem.h>
#include <Network/ENetSystem.h>
#include "RConServer.h"
#include "ServerListing.h"
#include "ServerConfiguration.h"

#define MAX_CHAT_MESSAGE_LENGTH 4096
#define MAX_COMMAND_LENGTH MAX_CHAT_MESSAGE_LENGTH
#define MAX_COMMAND_MESSAGE_LENGTH MAX_COMMAND_LENGTH

class CUnhandledCommandHandler2 : public CUnhandledCommandHandler
{
public:
	virtual void Execute(const GChar* pszCommandName, const GChar* pszArguments, CBaseObject* pClient) override;
};

class CBaseServer : public CNetCompatibilityShim
{
public:
	CBaseServer(Galactic3D::Context* pContext);
	virtual ~CBaseServer();

	struct tRule
	{
		GString m_Key;
		GString m_Value;
	};

	CLockable m_Lock;
	size_t m_MaxClients;
	Galactic3D::Context* m_pContext;
	Galactic3D::Signalable m_ExitSignal;
	uint32_t m_uiNetVersion;
#ifdef _DEBUG
	uint32_t m_uiFakeNetVersion;
#endif
	FILE* m_pLog;
	CServerResourceMgr m_ResourceMgr;
	EventHandlers::CEventType* m_pOnPlayerConnectEventType;
	EventHandlers::CEventType* m_pOnPlayerJoinEventType;
	EventHandlers::CEventType* m_pOnPlayerJoinedEventType;
	EventHandlers::CEventType* m_pOnPlayerQuitEventType;
	EventHandlers::CEventType* m_pOnPlayerChatEventType;
	EventHandlers::CEventType* m_pOnPlayerCommandEventType;
	EventHandlers::CEventType* m_pOnProcessEventType;
	EventHandlers::CEventType* m_pOnServerStartEventType;
	bool m_bExit;
	bool m_bServerBrowser;
	bool m_bRCon;
	bool m_bHttpServer;
	bool m_bServerQuery;
	uint16_t m_usPort;
	uint16_t m_usHTTPPort;
	GString m_HTTPUrl;
	uint16_t m_usRConPort;
	uint16_t m_usSyncInterval;
	eSyncMethod m_SyncMethod;
	bool m_bDuplicateNames;
	bool m_bMultiThreaded;
	uint32_t m_uiMinMajorVersion;
	uint32_t m_uiMinMinorVersion;
	uint32_t m_uiMinPatchVersion;
	uint32_t m_uiMinBuildVersion;
	GChar m_szLogPath[64];
	GChar m_szLogTimeStamp[64];
	GChar m_szBindIP[64];
	GChar m_szServerName[64];
	GChar m_szGameMode[64];
	GChar m_szMap[64];
	GChar m_szServerListing[64];
	CSecurePassword m_Password;
	bool m_bAllowModdedExecutables;
	size_t m_CurrentClients;
	Strong<CNetMachine> m_pConsole;
	CServerManager* m_pManager;
	TimeManager m_TimeManager;
	CRConServer m_RCon;
	CMasterlistAnnouncer m_Announcer;
	CHTTPMgr m_HTTPMgr;
	GString m_Game;
	int32_t m_GameId;
	std::vector<int32_t> m_AllowedGameIds;
private:
	std::vector<GString> m_InitialResources;
	CThreadCB* m_pInputThread;
	CPumpedEventLoop m_InputEvent;
public:
	CBanList m_BanList;
	CVarSystem m_CVars;
	std::vector<tRule> m_Rules;
	CUnleashedGameProtocolHandler m_UGP;
private:
	int32_t m_iWaitEventTimeout;
	ePacketPriority m_SyncPacketPriority;
	ePacketFlags m_SyncPacketFlags;
public:
	CHttpServer m_HttpServer;
	CNetMachines m_NetMachines;
	CNetRPC m_NetRPC;
	CConfig m_ServerConfig;

	float m_fStreamInDistance;
	float m_fStreamOutDistance;

protected:
	void RegisterFunctions(CScripting* pScripting);

public:
	bool IsVersionAllowed(uint32_t Major, uint32_t Minor, uint32_t Patch, uint32_t Build);

	virtual CNetMachine* NewMachine(CServerManager* pServerManager);
	virtual bool OnPlayerConnect(const Peer_t Peer) override;
	virtual void OnPlayerDisconnect(const Peer_t Peer, unsigned int uiReason) override;
	virtual void ProcessPacket(const tPeerInfo& Peer, unsigned int PacketID, Galactic3D::Stream* pStream) override;
	virtual bool ReceiveDatagram(CNetSocket* pNetSocket) override;

	void SetGame(const GChar* pszName);
	void AddGame(const GChar* pszName);
	void SendSync(CNetMachine* pNetMachine);
	virtual void ManageElements(CNetMachine* pNetMachine);
	void SendAllSync();
	void SendChat(CNetMachine* pNetMachine, const GChar* pszMessage, size_t MessageLength, unsigned int uiColour);
	void SendChatExcept(CNetMachine* pNetMachine, const GChar* pszMessage, size_t MessageLength, unsigned int uiColour);
	void UserChat(CNetMachine* pNetMachine, const GChar* pszMessage, size_t MessageLength);
	virtual bool ParseConfig(const CServerConfiguration& Config);
	bool LoadConfig(const GChar* pszPath);
	virtual void RegisterConfig();
	bool StartInitialResources();
	bool StartServer();
	void ShutDown();
	void StartInputThread();
	bool StartMasterlist();
	bool OnFrame();
	void MainLoop();
	void InputThread();

	const GChar* GetGameMode() { return m_szGameMode; }
	void SetGameMode(const GChar* pszGameMode);
	void SendGameMode(CNetMachine* pNetMachine);

	const GChar* GetServerName() { return m_szServerName; }
	void SetServerName(const GChar* pszName);
	void SendServerName(CNetMachine* pNetMachine);
	void SendCVars(CNetMachine* pNetMachine);

	virtual void OnProcess(const FrameTimeInfo* pTime);
	virtual void OnPlayerJoin(CNetMachine* pNetMachine);
	virtual void OnPlayerJoined(CNetMachine* pNetMachine);

	void ProcessConsoleInput(const GChar* pszCommandString);

	bool IsNameInUse(const GChar* pszName);

	void SetRule(const GChar* pszKey, const GChar* pszValue);
	const GChar* GetRule(const GChar* pszKey);
};
