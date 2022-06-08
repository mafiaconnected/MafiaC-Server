#pragma once

#include <vector>
#include <Multiplayer/UnleashedGameProtocol.h>

class CServer;

class CUnleashedGameProtocolHandler
{
public:
	CUnleashedGameProtocolHandler(CServer* pServer);
	~CUnleashedGameProtocolHandler();

	CServer* m_pServer;
	CLockable m_Lock;
	CUGPResponse* m_pResponse;

	void UpdateResponse();
	bool ReceiveDatagram(CNetSocket* pNetSocket);
};
