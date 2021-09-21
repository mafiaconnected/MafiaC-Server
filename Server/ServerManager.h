#pragma once

#include <Multiplayer/Manager.h>
#include <Multiplayer/NetObject.h>
#include <Multiplayer/NetObjectMgr.h>
#include "ServerResourceMgr.h"

class CServer;

class CServerManager : public CManager, public CNetObjectMgr
{
public:
	CServerManager(Context* pContext, CServer* pServer);

	CServer* m_pServer;

protected:
	void RegisterFunctions(CScripting* pScripting);
};
