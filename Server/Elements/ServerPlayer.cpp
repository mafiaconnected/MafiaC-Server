
#include "pch.h"
#include "Server.h"
#include "MafiaServerManager.h"

CServerPlayer::CServerPlayer(CMafiaServerManager* pServerManager) : CServerHuman(pServerManager)
{
	m_Type = ELEMENT_PLAYER;
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
