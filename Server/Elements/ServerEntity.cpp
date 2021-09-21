
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

	CBinaryReader Reader(pStream);
	Reader.ReadVector3D(&m_Position, 1);
	Reader.ReadVector3D(&m_Rotation, 1);

	m_RelPosition = m_Position;
	m_RelRotation = m_Rotation;

	size_t size;
	_gstrcpy_s(m_szModel, ARRAY_COUNT(m_szModel), Reader.ReadString(&size));

	return true;
}

bool CServerEntity::ReadSyncPacket(Stream* pStream)
{
	if (!CNetObject::ReadSyncPacket(pStream))
		return false;

	CBinaryReader Reader(pStream);

	Reader.ReadVector3D(&m_Position, 1);
	Reader.ReadVector3D(&m_RelPosition, 1);
	Reader.ReadVector3D(&m_Rotation, 1);
	Reader.ReadVector3D(&m_RelRotation, 1);

	CNetObject::SetPosition(m_Position);
	CNetObject::SetRotation(m_Rotation);

	return true;
}

bool CServerEntity::WriteCreatePacket(Stream* pStream)
{
	if (!CNetObject::WriteCreatePacket(pStream))
		return false;

	CBinaryWriter Writer(pStream);

	Writer.WriteString(m_szModel);
	Writer.WriteVector3D(&m_Position, 1);
	Writer.WriteVector3D(&m_RelPosition, 1);
	Writer.WriteVector3D(&m_Rotation, 1);
	Writer.WriteVector3D(&m_RelRotation, 1);
	return true;
}

bool CServerEntity::WriteSyncPacket(Stream* pStream)
{
	if (!CNetObject::WriteSyncPacket(pStream))
		return false;

	CBinaryWriter Writer(pStream);

	Writer.WriteVector3D(&m_Position, 1);
	Writer.WriteVector3D(&m_Rotation, 1);
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