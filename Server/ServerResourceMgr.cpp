
#include "pch.h"
#include "ServerResourceMgr.h"
#include "MafiaServerManager.h"
#include <mongoose.h>
#include "BaseServer.h"

using namespace Galactic3D;

CServerResourceMgr::CServerResourceMgr(Galactic3D::Context* pContext, CBaseServer* pServer) : CNetGameResourceMgr(pContext)
{
	m_pServer = pServer;
	m_bIsServer = true;
	m_pCommandHandlers->m_bClientThere = true;
	m_pNetworkHandlers->m_bClientThere = true;
	m_pKeyBinds->m_bClientThere = true;
}

void CServerResourceMgr::UpdateAResource(CNetCompatibilityShim* pNetGame, CResource* pResource)
{
	Packet Packet(PACKET_RESOURCES);

	Packet.Write<Uint32>(1); // Count
	pResource->WriteClientInfo(&Packet);

	pNetGame->SendEveryonePacket(&Packet, PACKETPRIORITY_LOW, PACKETFLAGS_RELIABLE);
}

void CServerResourceMgr::UpdateAllResource(CNetCompatibilityShim* pNetGame, const Peer_t Peer)
{
	Packet Packet(PACKET_RESOURCES);

	WriteClientInfo(&Packet);

	pNetGame->SendPacket(Peer, &Packet, PACKETPRIORITY_LOW, PACKETFLAGS_RELIABLE);
}

void CServerResourceMgr::RefreshResourceState(CResource* pResource)
{
	if (pResource->IsStarted())
		UpdateAResource(m_pServer, pResource);
	else
	{
		Packet Packet(PACKET_REMOVERESOURCE);

		uint32_t uiHash = CRC32Hash::GetHash(pResource->m_Name.c_str(), pResource->m_Name.size(), true);
		Packet.Write<uint32_t>(uiHash);

		m_pServer->SendEveryonePacket(&Packet, PACKETPRIORITY_LOW, PACKETFLAGS_RELIABLE);
	}
}

void CServerResourceMgr::RemoveThingsAssociatedWithResource(CResource* pResource)
{
	for (size_t i=0; i<m_pServer->m_pManager->m_Objects.GetSize(); i++)
	{
		if (m_pServer->m_pManager->m_Objects.IsUsedAt(i))
		{
			CNetObject* pObject = m_pServer->m_pManager->m_Objects.GetAt(i);

			if (pObject != nullptr)
			{
				if (pObject->m_pResource == pResource)
				{
					m_pServer->m_pManager->DestroyObject(pObject);
				}
			}
		}
	}

	CNetGameResourceMgr::RemoveThingsAssociatedWithResource(pResource);
}
