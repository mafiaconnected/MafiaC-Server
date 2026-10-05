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

	// A transient element its syncer dropped while other machines still have it: hand it to one of them (true),
	// or remove it for everyone (false)
	virtual bool ShouldHandOffDroppedElement(CNetObject* pObject) { return true; }

protected:
	void RegisterFunctions(CScripting* pScripting);
};
