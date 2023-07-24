
#include "pch.h"
#include "MafiaServer.h"
#include "MafiaServerManager.h"
#include "ServerDummy.h"

#include "Utils/VectorTools.h"

CServerDummy::CServerDummy(CMafiaServerManager* pServerManager) : CServerEntity(pServerManager)
{
	m_Type = ELEMENT_DUMMY;

	//m_Flags.m_bFindSyncer = true;
	//m_Flags.m_bSendSync = true;
	m_Flags.m_bDistanceStreaming = true;

	m_fStreamInDistance = pServerManager->m_pServer->m_fStreamInDistance;
	m_fStreamOutDistance = pServerManager->m_pServer->m_fStreamOutDistance;
}

ReflectedClass* CServerDummy::GetReflectedClass()
{
	return static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pServerDummyClass;
}