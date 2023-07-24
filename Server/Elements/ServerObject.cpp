
#include "pch.h"
#include "MafiaServer.h"
#include "MafiaServerManager.h"
#include "ServerObject.h"

#include "Utils/VectorTools.h"

CServerObject::CServerObject(CMafiaServerManager* pServerManager) : CServerEntity(pServerManager)
{
	m_Type = ELEMENT_OBJECT;

	//m_Flags.m_bFindSyncer = true;
	//m_Flags.m_bSendSync = true;
	m_Flags.m_bDistanceStreaming = true;

	m_fStreamInDistance = pServerManager->m_pServer->m_fStreamInDistance;
	m_fStreamOutDistance = pServerManager->m_pServer->m_fStreamOutDistance;
}

ReflectedClass* CServerObject::GetReflectedClass()
{
	return static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pServerObjectClass;
}