#pragma once

#include <vector>
#include <Multiplayer/UnleashedGameProtocol.h>

class CServer;

class CUnleashedGameProtocolHandler : public CNetDatagramHandler
{
public:
	CUnleashedGameProtocolHandler(CServer* pServer);
	~CUnleashedGameProtocolHandler();

	CServer* m_pServer;
	CLockable m_Lock;
	CUGPResponse* m_pResponse;

	void UpdateResponse();
	virtual bool ReceiveDatagram(CNetSocket* pNetSocket) override;
};
