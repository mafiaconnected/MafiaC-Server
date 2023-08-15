
#include "pch.h"
#include "MafiaServer.h"
#include "MafiaServerManager.h"
#include "ServerDummy.h"

#include "Utils/VectorTools.h"

CServerDummy::CServerDummy(CMafiaServerManager* pServerManager) : CNetObject(pServerManager)
{
	m_Type = ELEMENT_DUMMY;

	m_Flags.m_bFindSyncer = false;
	m_Flags.m_bSendSync = false;
	m_Flags.m_bDistanceStreaming = true;

	m_fStreamInDistance = pServerManager->m_pServer->m_fStreamInDistance;
	m_fStreamOutDistance = pServerManager->m_pServer->m_fStreamOutDistance;
}

ReflectedClass* CServerDummy::GetReflectedClass()
{
	return static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pServerDummyClass;
}

bool CServerDummy::SetPosition(const CVector3D& vecPos)
{
	m_Position = vecPos;

	return true;
}

bool CServerDummy::GetPosition(CVector3D& vecPos)
{
	vecPos = m_Position;

	return true;
}