
#include "pch.h"
#include "Server.h"
#include "MafiaServerManager.h"

CServerPlayer::CServerPlayer(CMafiaServerManager* pServerManager) : CServerHuman(pServerManager)
{
	m_Type = ELEMENT_PLAYER;

	m_Flags.m_bAlwaysExistForSyncer = true;
}

ReflectedClass* CServerPlayer::GetReflectedClass(void)
{
	return static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pServerPlayerClass;
}

bool CServerPlayer::ReadSyncPacket(Stream* pStream)
{
	if (!CServerHuman::ReadSyncPacket(pStream))
		return false;

	return true;
}

bool CServerPlayer::WriteSyncPacket(Stream* pStream)
{
	if (!CServerHuman::WriteSyncPacket(pStream))
		return false;

	return true;
}

bool CServerPlayer::SetPosition(const CVector3D& vecPos)
{
	if (!CServerHuman::SetPosition(vecPos))
		return false;

	//Packet Packet(MAFIAPACKET_PLAYER_SETPOSITION);
	//{
	//	CBinaryWriter Writer(&Packet);
	//	Writer.WriteInt32(GetId());
	//	Writer.WriteVector3D(vecPos);
	//}

	//m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);

	return true;
}
