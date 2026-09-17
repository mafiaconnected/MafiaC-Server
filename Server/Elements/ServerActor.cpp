
#include "pch.h"
#include "MafiaServer.h"
#include "MafiaServerManager.h"
#include "ServerActor.h"

#include "Utils/VectorTools.h"

CServerActor::CServerActor(CMafiaServerManager* pServerManager) : CServerEntity(pServerManager)
{
	m_Type = ELEMENT_ACTOR;
}

ReflectedClass* CServerActor::GetReflectedClass()
{
	return static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pServerActorClass;
}
