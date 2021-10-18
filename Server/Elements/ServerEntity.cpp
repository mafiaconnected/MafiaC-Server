
#include "pch.h"
#include "Server.h"
#include "MafiaServerManager.h"
#include "Utils/VectorTools.h"

CServerEntity::CServerEntity(CMafiaServerManager* pServerManager) : CNetObject(pServerManager)
{
	m_Type = ELEMENT_ENTITY;
	m_Position = { 0, 0, 0 };
	m_Rotation = { 0, 0, 0 };
	//m_szModel = L"";
	m_nRelativeElement = INVALID_NETWORK_ID;
	m_nRef = -1;
	m_fStreamInDistance = pServerManager->m_pServer->m_fStreamInDistance;
	m_fStreamOutDistance = pServerManager->m_pServer->m_fStreamOutDistance;
	m_pServerManager = pServerManager;
}

void CServerEntity::SetHeading(float fHeading)
{
	m_Rotation = CVecTools::ComputeDirEuler(fHeading);
}

float CServerEntity::GetHeading(void)
{
	return CVecTools::DirToRotation180(CVecTools::EulerToDir(m_Rotation));
}

ReflectedClass* CServerEntity::GetReflectedClass(void)
{
	return static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pServerEntityClass;
}

bool CServerEntity::SetPosition(const CVector3D& vecPos)
{
	CNetObject::SetPosition(vecPos);

	m_Position = vecPos;

	return true;
}

bool CServerEntity::GetPosition(CVector3D& vecPos)
{
	vecPos = m_Position;

	return true;
}

bool CServerEntity::SetRotation(const CVector3D& vecRotation)
{
	CNetObject::SetRotation(vecRotation);

	m_Rotation = vecRotation;

	return true;
}

bool CServerEntity::GetRotation(CVector3D& vecRotation)
{
	vecRotation = m_Rotation;

	return true;
}

bool CServerEntity::ShouldExistForMachine(CNetMachine* pClient)
{
	auto pPlayer = pClient->GetPlayer();
	if (this == pPlayer)
		return true;
	if (m_nRef != -1 && GetSyncer() == pClient->m_nIndex)
		return true;
	if (pPlayer != NULL && pPlayer->IsType(ELEMENT_ENTITY))
	{
		auto pServer = static_cast<CServerManager*>(m_pNetObjectMgr)->m_pServer;
		CServerEntity* pPlayerEntity = static_cast<CServerEntity*>(pPlayer);
		float fDistance = (m_Position - pPlayerEntity->m_Position).GetLength();
		if (fDistance < m_fStreamInDistance)
			return true;
	}
	return false;
}

bool CServerEntity::ShouldDeleteForMachine(CNetMachine* pClient)
{
	auto pPlayer = pClient->GetPlayer();
	if (this == pPlayer)
		return false;
	if (m_nRef != -1 && GetSyncer() == pClient->m_nIndex)
		return false;
	if (pPlayer != NULL && pPlayer->IsType(ELEMENT_ENTITY))
	{
		auto pServer = static_cast<CServerManager*>(m_pNetObjectMgr)->m_pServer;
		CServerEntity* pPlayerEntity = static_cast<CServerEntity*>(pPlayer);
		float fDistance = (m_Position - pPlayerEntity->m_Position).GetLength();
		if (fDistance > m_fStreamOutDistance)
			return true;
	}
	return false;
}

bool CServerEntity::ReadCreatePacket(Stream* pStream)
{
	if (!CNetObject::ReadCreatePacket(pStream))
		return false;

	tEntityCreatePacket Packet;

	if (pStream->Read(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	_gstrcpy_s(m_szModel, ARRAY_COUNT(m_szModel), Packet.model);

	m_Position = Packet.position;
	m_RelPosition = Packet.positionRel;
	m_Rotation = Packet.rotation;
	m_RelRotation = Packet.rotationRel;

	SetPosition(m_Position);
	SetRotation(m_Rotation);

	//m_RelPosition = m_Position;
	//m_RelRotation = m_Rotation;

	return true;
}

bool CServerEntity::ReadSyncPacket(Stream* pStream)
{
	if (!CNetObject::ReadSyncPacket(pStream))
		return false;

	tEntitySyncPacket Packet;

	if (pStream->Read(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	m_Position = Packet.position;
	m_RelPosition = Packet.positionRel;
	m_Rotation = Packet.rotation;
	m_RelRotation = Packet.rotationRel;

	SetPosition(m_Position);
	SetRotation(m_Rotation);

	return true;
}

bool CServerEntity::WriteCreatePacket(Stream* pStream)
{
	if (!CNetObject::WriteCreatePacket(pStream))
		return false;

	tEntityCreatePacket Packet;

	_gstrcpy_s(Packet.model, ARRAY_COUNT(Packet.model), m_szModel);

	Packet.position = m_Position;
	Packet.positionRel = m_RelPosition;
	Packet.rotation = m_Rotation;
	Packet.rotationRel = m_RelRotation;

	if (pStream->Write(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	return true;
}

bool CServerEntity::WriteSyncPacket(Stream* pStream)
{
	if (!CNetObject::WriteSyncPacket(pStream))
		return false;

	tEntitySyncPacket Packet;

	Packet.position = m_Position;
	Packet.positionRel = m_RelPosition;
	Packet.rotation = m_Rotation;
	Packet.rotationRel = m_RelRotation;

	if (pStream->Write(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	return true;
}

bool CServerEntity::IsRandom()
{
	if (IsType(ELEMENT_PLAYER))
		return false;
	if (m_pResource != nullptr)
		return false;
	return true;
}

void CServerEntity::SetModel(const GChar* modelName)
{
	_gstrcpy_s(m_szModel, ARRAY_COUNT(m_szModel), modelName);
}