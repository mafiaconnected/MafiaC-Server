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
#include <Engine/Config.h>

#define MAX_CHAT_MESSAGE_LENGTH 4096
#define MAX_COMMAND_LENGTH MAX_CHAT_MESSAGE_LENGTH
#define MAX_COMMAND_MESSAGE_LENGTH MAX_COMMAND_LENGTH

class CServer;
class CServerHuman;
class CServerVehicle;
class CServerPlayer;
class CMafiaServerManager;

class CRConServer : public WebConnection
{
public:
	CRConServer(CServer* pServer);

	CServer* m_pServer;
	CSecurePassword m_Password;

	virtual void ProcessPacket(mg_connection* nc, unsigned int PacketID, Stream* pStream) override;
	virtual void EventHandler(struct mg_connection* nc, int ev, void* ev_data) override;

	void Log(mg_connection* nc, eLogType LogType, const GChar* psz);
	void Auth(mg_connection* nc, bool b);
};

enum eMasterlistState
{
	MASTERLIST_NONE,
	MASTERLIST_CONNECTING,
	MASTERLIST_CONNECTED,
	MASTERLIST_DISCONNECTED,
	MASTERLIST_DEAD,
};

class CMasterlistAnnouncer : public Galactic3D::WebConnection
{
public:
	CMasterlistAnnouncer(CServer* pServer);

protected:
	CServer* m_pServer;
	float m_fRetryTime;
	eMasterlistState m_OldState;
	eMasterlistState m_State;

	void ChangeState(eMasterlistState State);

public:
	void Pulse(float fDeltaTime);
	virtual void EventHandler(struct mg_connection* nc, int ev, void* ev_data) override;
	virtual void ProcessPacket(mg_connection* nc, unsigned int PacketID, Stream* pStream) override;

	void PlayerAdd(CNetMachine* pNetMachine);
	void PlayerRemove(CNetMachine* pNetMachine);
	void SetGameMode(const GChar* pszGameMode, size_t Length);
	void SetName(const GChar* pszName, size_t Length);

	virtual void OnPlayerConnect(mg_connection *nc) override;
	virtual void OnPlayerDisconnect(mg_connection *nc) override;
};

class CServerConfiguration
{
public:
	CServerConfiguration();
	~CServerConfiguration();

	CElementChunk* m_pConfig;

	bool GetBoolValue(const GChar* pszName, bool bDefault) const;
	const GChar* GetStringValue(const GChar* pszName, const GChar* pszDefault) const;
	int32_t GetInt32Value(const GChar* pszName, int32_t iDefault) const;
	float GetFloatValue(const GChar* pszName, float fDefault) const;

	bool Read(Stream* pStream);
};

class CUnhandledCommandHandler2 : public CUnhandledCommandHandler
{
public:
	virtual void Execute(const GChar* pszCommandName, const GChar* pszArguments, CBaseObject* pClient) override;
};

class CServer : public CNetCompatibilityShim
{
public:
	CServer(Galactic3D::Context* pContext);
	~CServer(void);

	struct tRule
	{
		GString m_Key;
		GString m_Value;
	};

	CLockable m_Lock;
	size_t m_MaxClients;
	Galactic3D::Context* m_pContext;
	Galactic3D::Signalable m_ExitSignal;
	uint32_t m_uiVersionMin;
	uint32_t m_uiVersionMax;
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
	unsigned short m_usPort;
	unsigned short m_usHTTPPort;
	GString m_HTTPUrl;
	unsigned short m_usRConPort;
	unsigned short m_usSyncInterval;
	eSyncMethod m_SyncMethod;
	bool m_bDuplicateNames;
	uint32_t m_uiMinMajorVersion;
	uint32_t m_uiMinMinorVersion;
	uint32_t m_uiMinPatchVersion;
	uint32_t m_uiMinBuildVersion;
	GChar m_szLogPath[64];
	GChar m_szLogTimeStamp[64];
	GChar m_szBindIP[64];
	GChar m_szServerName[64];
	GChar m_szGameMode[64];
	GChar m_szLevel[64];
	GChar m_szServerListing[64];
	CSecurePassword m_Password;
	bool m_bAllowModdedExecutables;
	size_t m_CurrentClients;
	Strong<CNetMachine> m_pConsole;
	CMafiaServerManager* m_pManager;
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

	float m_fPickupStreamInDistance;
	float m_fPickupStreamOutDistance;

	bool m_bSyncLocalEntities;
	bool m_bPlanes;
	bool m_bTrains;
	bool m_bStuntJumps;
	bool m_bUniqueStuntJumps;
	bool m_bGunShops;
	bool m_bCamera;
	bool m_bNametags;
	bool m_bTimeSync;
	bool m_bWeatherSync;

protected:
	void RegisterFunctions(CScripting* pScripting);

public:
	bool IsVersionAllowed(uint32_t Major, uint32_t Minor, uint32_t Patch, uint32_t Build);

	virtual bool OnPlayerConnect(const Peer_t Peer) override;
	virtual void OnPlayerDisconnect(const Peer_t Peer, unsigned int uiReason) override;
	virtual void ProcessPacket(const tPeerInfo& Peer, unsigned int PacketID, Galactic3D::Stream* pStream) override;

	void SetGame(const GChar* pszName);
	void AddGame(const GChar* pszName);
	void SendSync(CNetMachine* pNetMachine);
	void ManageElements(CNetMachine* pNetMachine);
	void SendAllSync(void);
	void SendChat(CNetMachine* pNetMachine, const GChar* pszMessage, size_t MessageLength, unsigned int uiColour);
	void SendChatExcept(CNetMachine* pNetMachine, const GChar* pszMessage, size_t MessageLength, unsigned int uiColour);
	void UserChat(CNetMachine* pNetMachine, const GChar* pszMessage, size_t MessageLength);
	bool ParseConfig(const CServerConfiguration& Config);
	bool LoadConfig(const GChar* pszPath);
	bool StartInitialResources();
	bool StartServer(void);
	void ShutDown();
	void StartInputThread();
	bool StartMasterlist(void);
	bool OnFrame(void);
	void MainLoop(void);
	void InputThread();

	const GChar* GetGameMode(void) { return m_szGameMode; }
	void SetGameMode(const GChar* pszGameMode);
	void SendGameMode(CNetMachine* pNetMachine);

	const GChar* GetServerName(void) { return m_szServerName; }
	void SetServerName(const GChar* pszName);
	const GChar* GetMapName(void) { return m_szLevel; }
	void SetMapName(const GChar* pszName);
	void SendServerName(CNetMachine* pNetMachine);
	void SendMapName(CNetMachine* pNetMachine);
	void SendCVars(CNetMachine* pNetMachine);

	void OnProcess(const FrameTimeInfo* pTime);
	void OnPlayerJoin(CNetMachine* pNetMachine);
	void OnPlayerJoined(CNetMachine* pNetMachine);

	void ProcessConsoleInput(const GChar* pszCommandString);

	bool IsNameInUse(const GChar* pszName);

	void SetRule(const GChar* pszKey, const GChar* pszValue);
	const GChar* GetRule(const GChar* pszKey);

	void SyncroniseEnterVehicle(CServerHuman* pHuman, CServerVehicle* pVehicle, bool bDriver);
	void SyncroniseExitVehicle(CServerHuman* pHuman);
};
