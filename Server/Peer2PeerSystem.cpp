
#include "pch.h"
#include "MafiaServer.h"
#include "MafiaServerManager.h"
#include "Peer2PeerSystem.h"
#include "Elements/Elements.h"

CPeer2PeerSystem::CPeer2PeerSystem(CMafiaServerManager* pManager) :
	m_pManager(pManager)
{
}

void CPeer2PeerSystem::ProcessPacket(const tPeerInfo& Peer, unsigned int PacketID, Galactic3D::Stream* pStream)
{
	Galactic3D::CBinaryReader Reader(pStream);

	if (!m_pManager->m_pMafiaServer->m_bSyncLocalEntities)
		return;

	auto pClient = static_cast<CMafiaClient*>(m_pManager->m_pNetMachines->GetMachineFromPeer(Peer.m_Peer));

	switch (PacketID)
	{
		case MAFIAPACKET_PEER_CREATECAR:
			{
				Sint32 nRef;
				Reader.ReadInt32(&nRef, 1);
				uint64_t Nonce;
				Reader.ReadUInt64(&Nonce, 1);

				auto pVehicle = Strong<CServerVehicle>::New(m_pManager->Create(ELEMENT_VEHICLE));
				pVehicle->m_nRef = nRef;
				pVehicle->m_Flags.m_bAlwaysExistForSyncer = true;
				pVehicle->m_Flags.m_bTransient = true;
				pVehicle->ReadCreatePacket(pStream);
				pVehicle->ReadSyncPacket(pStream);
				if (pVehicle->m_ucCreatedBy == ELEMENTCREATEDBY_POPULATION)
				{
					CVector3D vecPos;
					pVehicle->GetPosition(vecPos);
					if (m_pManager->IsAnythingBlocking(vecPos))
					{
						//_glogwarnprintf(_gstr("PEER2PEER: Rejected vehicle %d"), nRef);

						Packet Packet(MAFIAPACKET_PEER_IDENTIFY);
						Packet.Write<Sint32>(ELEMENT_VEHICLE);
						Packet.Write<Sint32>(nRef);
						Packet.Write<uint64_t>(Nonce);
						Packet.Write<int32_t>(INVALID_NETWORK_ID);
						pClient->SendPacket(&Packet);

						return;
					}
				}
				if (m_pManager->RegisterNetObject(pVehicle))
				{
					//_glogprintf(_gstr("PEER2PEER: Vehicle %d assigned to %d"), nRef, pVehicle->GetId());

					Packet Packet(MAFIAPACKET_PEER_IDENTIFY);
					Packet.Write<Sint32>(ELEMENT_VEHICLE);
					Packet.Write<Sint32>(nRef);
					Packet.Write<uint64_t>(Nonce);
					Packet.Write<int32_t>(pVehicle->GetId());
					pClient->SendPacket(&Packet);

					pVehicle->SetCreatedFor(pClient, true);
					pVehicle->SetSyncer(pClient, false);
				}
			}
			break;

		case MAFIAPACKET_PEER_CREATECIVILIAN:
			{
				Sint32 nRef;
				Reader.ReadInt32(&nRef, 1);
				uint64_t Nonce;
				Reader.ReadUInt64(&Nonce, 1);

				auto pPed = Strong<CServerHuman>::New(m_pManager->Create(ELEMENT_PED));
				pPed->m_nRef = nRef;
				pPed->m_Flags.m_bAlwaysExistForSyncer = true;
				pPed->m_Flags.m_bTransient = true;
				pPed->ReadCreatePacket(pStream);
				pPed->ReadSyncPacket(pStream);
				if (m_pManager->RegisterNetObject(pPed))
				{
					//_glogprintf(_gstr("PEER2PEER: Civilian %d assigned to %d"), nRef, pPed->GetId());

					Packet Packet(MAFIAPACKET_PEER_IDENTIFY);
					Packet.Write<Sint32>(ELEMENT_PED);
					Packet.Write<Sint32>(nRef);
					Packet.Write<uint64_t>(Nonce);
					Packet.Write<int32_t>(pPed->GetId());
					pClient->SendPacket(&Packet);

					pPed->SetCreatedFor(pClient, true);
					pPed->SetSyncer(pClient, false);
				}
			}
			break;

#if 0
		case MAFIAPACKET_PEER_REMOVEREFS:
			{
				int32_t nRef;
				Reader.ReadInt32(&nRef, 1);

				auto pElement = Find(nRef, pClient);
				if (pElement != nullptr)
				{
					Packet Packet(MAFIAPACKET_PEER_REMOVEREFS);
					Packet.Write<int32_t>(pElement->GetId());
					m_pManager->m_pServer->SendObjectRelatedPacket(&Packet, pElement);
				}
			}
			break;
#endif
	}
}
