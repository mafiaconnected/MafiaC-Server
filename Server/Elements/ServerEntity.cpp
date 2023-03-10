
#include "pch.h"
#include "Server.h"
#include "MafiaServerManager.h"
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
	//m_szModel = L"";
	m_nRelativeElement = INVALID_NETWORK_ID;
	m_nRef = -1;
	m_pServerManager = pServerManager;
}

void CServerEntity::SetHeading(float fHeading)
{
	m_Rotation = CVecTools::ComputeDirEuler(CVecTools::RadToDeg(fHeading));
}

float CServerEntity::GetHeading(void)
{
	return CVecTools::DegToRad(CVecTools::DirToRotation360(CVecTools::EulerToDir(m_Rotation)));
}

ReflectedClass* CServerEntity::GetReflectedClass()
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

bool CServerEntity::SetVelocity(const CVector3D& vecVelocity)
{
	m_RelPosition = vecVelocity;

	return true;
}

bool CServerEntity::GetVelocity(CVector3D& vecVelocity)
{
	vecVelocity = m_RelPosition;

	return true;
}

bool CServerEntity::SetRotationVelocity(const CVector3D& vecRotationVelocity)
{
	m_RelRotation = vecRotationVelocity;

	return true;
}

bool CServerEntity::GetRotationVelocity(CVector3D& vecRotationVelocity)
{
	vecRotationVelocity = m_RelRotation;

	return true;
}

bool CServerEntity::ReadCreatePacket(Stream* pStream)
{
	if (!CNetObject::ReadCreatePacket(pStream))
		return false;

	tEntityCreatePacket Packet;

	if (pStream->Read(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	_gstrcpy_s(m_szModel, ARRAY_COUNT(m_szModel), Packet.model);

	// Check for NaN. Mafia sometimes has them. Once synced, it streams the element out for players (even a driver's own vehicle disappears)
	if (Packet.position.x != NAN && Packet.position.y != NAN && Packet.position.z != NAN) {
		m_Position = Packet.position;
	}
	
	if (Packet.positionRel.x != NAN && Packet.positionRel.y != NAN && Packet.positionRel.z != NAN) {
		m_RelPosition = Packet.positionRel;
	}

	if (Packet.rotation.x != NAN && Packet.rotation.y != NAN && Packet.rotation.z != NAN) {
		m_Rotation = Packet.rotation;
	}
	
	if (Packet.rotationRel.x != NAN && Packet.rotationRel.y != NAN && Packet.rotationRel.z != NAN) {
		m_RelRotation = Packet.rotationRel;
	}

	return true;
}

bool CServerEntity::ReadSyncPacket(Stream* pStream)
{
	if (!CNetObject::ReadSyncPacket(pStream))
		return false;

	tEntitySyncPacket Packet;

	if (pStream->Read(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	// Check for NaN. Mafia sometimes has them. Once synced, it streams the element out for players (even a driver's own vehicle disappears)
	if (Packet.position.x != NAN && Packet.position.y != NAN && Packet.position.z != NAN) {
		m_Position = Packet.position;
	}

	if (Packet.positionRel.x != NAN && Packet.positionRel.y != NAN && Packet.positionRel.z != NAN) {
		m_RelPosition = Packet.positionRel;
	}

	if (Packet.rotation.x != NAN && Packet.rotation.y != NAN && Packet.rotation.z != NAN) {
		m_Rotation = Packet.rotation;
	}

	if (Packet.rotationRel.x != NAN && Packet.rotationRel.y != NAN && Packet.rotationRel.z != NAN) {
		m_RelRotation = Packet.rotationRel;
	}

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