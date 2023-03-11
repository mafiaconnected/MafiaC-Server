#pragma once

#include <vector>
#include <Multiplayer/UnleashedGameProtocol.h>

class CBaseServer;

class CUnleashedGameProtocolHandler
{
public:
	CUnleashedGameProtocolHandler(CBaseServer* pServer);
	~CUnleashedGameProtocolHandler();

	CBaseServer* m_pServer;
	CLockable m_Lock;
	CUGPResponse* m_pResponse;

	void UpdateResponse();
	bool ReceiveDatagram(CNetSocket* pNetSocket);
};
