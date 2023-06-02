
#include "pch.h"
#include "MafiaServer.h"
#include "MafiaServerManager.h"
#include "ServerEntity.h"

#include "Utils/VectorTools.h"

CServerEntity::CServerEntity(CMafiaServerManager* pServerManager) : CNetObject(pServerManager)
{
	m_Type = ELEMENT_ENTITY;

	m_Flags.m_bFindSyncer = true;
	m_Flags.m_bSendSync = true;
	m_Flags.m_bDistanceStreaming = true;

	m_fStreamInDistance = pServerManager->m_pServer->m_fStreamInDistance;
	m_fStreamOutDistance = pServerManager->m_pServer->m_fStreamOutDistance;

	m_Position = { 0, 0, 0 };
	m_Rotation = { 0, 0, 0 };
	m_nRelativeElement = INVALID_NETWORK_ID;
	m_nRef = -1;
	m_ucCreatedBy = ELEMENTCREATEDBY_USER;
}

ReflectedClass* CServerDummy::GetReflectedClass()
{
	return static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pServerDummyClass;
}

bool CServerDummy::SetPosition(const CVector3D& vecPos)
{
	CNetObject::SetPosition(vecPos);

	m_Position = vecPos;

	return true;
}

bool CServerDummy::GetPosition(CVector3D& vecPos)
{
	vecPos = m_Position;

	return true;
}

bool CServerDummy::ReadCreatePacket(Stream* pStream)
{
	if (!CNetObject::ReadCreatePacket(pStream))
		return false;

	tEntityCreatePacket Packet;

	if (pStream->Read(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	m_Position = Packet.position;

	return true;
}

bool CServerDummy::ReadSyncPacket(Stream* pStream)
{
	if (!CNetObject::ReadSyncPacket(pStream))
		return false;

	tEntitySyncPacket Packet;

	if (pStream->Read(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	m_Position = Packet.position;

	return true;
}

bool CServerDummy::WriteCreatePacket(Stream* pStream)
{
	if (!CNetObject::WriteCreatePacket(pStream))
		return false;

	tEntityCreatePacket Packet;

	Packet.position = m_Position;

	if (pStream->Write(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	return true;
}

bool CServerDummy::WriteSyncPacket(Stream* pStream)
{
	if (!CNetObject::WriteSyncPacket(pStream))
		return false;

	tEntitySyncPacket Packet;

	Packet.position = m_Position;

	if (pStream->Write(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	return true;
}