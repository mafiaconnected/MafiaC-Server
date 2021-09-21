#pragma once

#include <vector>
#include <Multiplayer/UnleashedGameProtocol.h>

class CServer;

class CUnleashedGameProtocolHandler : public CNetDatagramHandler
{
public:
	CUnleashedGameProtocolHandler(CServer* pServer);

	CServer* m_pServer;
	CLockable m_Lock;
	CUGPResponse m_Response;

	void UpdateResponse();
	virtual bool ReceiveDatagram(CNetSocket* pNetSocket) override;
};
