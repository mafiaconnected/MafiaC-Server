#pragma once

#include <Multiplayer/Manager.h>
#include <Multiplayer/NetObject.h>
#include <Multiplayer/NetObjectMgr.h>
#include "ServerResourceMgr.h"

class CBaseServer;

class CServerManager : public CManager, public CNetObjectMgr
{
public:
	CServerManager(Context* pContext, CBaseServer* pServer);

	CBaseServer* m_pServer;

protected:
	void RegisterFunctions(CScripting* pScripting);
};
