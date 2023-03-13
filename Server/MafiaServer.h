#pragma once

#include "BaseServer.h"

class CMafiaServer : public CBaseServer
{
public:
	CMafiaServer(Galactic3D::Context* pContext);
	virtual ~CMafiaServer();

	bool m_bSyncLocalEntities;

	GChar m_szMap[64];

	virtual CNetMachine* NewMachine(CServerManager* pServerManager) override;
	virtual void ProcessPacket(const tPeerInfo& Peer, unsigned int PacketID, Galactic3D::Stream* pStream) override;

	virtual bool ParseConfig(const CServerConfiguration& Config) override;

	virtual void OnProcess(const FrameTimeInfo* pTime) override;
	virtual void OnPlayerJoin(CNetMachine* pNetMachine) override;
	virtual void OnPlayerJoined(CNetMachine* pNetMachine) override;

	void SetMapName(const GChar* pszName);
	void SendMapName(CNetMachine* pNetMachine);
};
