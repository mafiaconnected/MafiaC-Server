
#include "pch.h"
#include "MafiaServer.h"
#include "MafiaServerManager.h"
#include "Peer2PeerSystem.h"
#include "Elements/Elements.h"

CPeer2PeerSystem::CPeer2PeerSystem(CMafiaServerManager* pManager) :
	m_pManager(pManager)
{
}

void CPeer2PeerSystem::ProcessPacket(Peer_t Peer, unsigned int PacketID, Galactic3D::Stream* pStream)
{
	Galactic3D::CBinaryReader Reader(pStream);

	if (!m_pManager->m_pMafiaServer->m_bSyncLocalEntities)
		return;

	auto pClient = static_cast<CMafiaClient*>(m_pManager->m_pNetMachines->GetMachineFromPeer(Peer));

	// Same reply the actor registration uses. The client matches it back up by GUID (see the
	// MAFIAPACKET_ELEMENT_UPDATE_ID handler in CMultiplayer::ProcessPacket) and ignores INVALID_NETWORK_ID.
	auto ReplyWithElementId = [&](uint64_t Guid, int32_t nId)
	{
		Packet Packet(MAFIAPACKET_ELEMENT_UPDATE_ID);
		Packet.Write<uint64_t>(Guid);
		Packet.Write<int32_t>(nId);
		pClient->SendPacket(&Packet);
	};

	switch (PacketID)
	{
		case MAFIAPACKET_PEER_CREATECAR:
			{
				uint64_t Guid;
				Reader.ReadUInt64(&Guid, 1);

				auto pVehicle = Strong<CServerVehicle>::New(m_pManager->Create(ELEMENT_VEHICLE));
				pVehicle->m_Flags.m_bAlwaysExistForSyncer = true;
				pVehicle->m_Flags.m_bTransient = true;
				if (!pVehicle->ReadCreatePacket(pStream) || !pVehicle->ReadSyncPacket(pStream))
				{
					ReplyWithElementId(Guid, INVALID_NETWORK_ID);
					return;
				}
				if (pVehicle->m_ucCreatedBy == ELEMENTCREATEDBY_POPULATION)
				{
					CVector3D vecPos;
					pVehicle->GetPosition(vecPos);
					if (m_pManager->IsAnythingBlocking(vecPos))
					{
						//_glogwarnprintf(_gstr("PEER2PEER: Rejected vehicle %llu"), Guid);

						ReplyWithElementId(Guid, INVALID_NETWORK_ID);

						return;
					}
				}
				if (m_pManager->RegisterNetObject(pVehicle))
				{
					//_glogprintf(_gstr("PEER2PEER: Vehicle %llu assigned to %d"), Guid, pVehicle->GetId());

					ReplyWithElementId(Guid, pVehicle->GetId());

					pVehicle->SetCreatedFor(pClient, true);
					pVehicle->SetSyncer(pClient, false);
				}
			}
			break;

		case MAFIAPACKET_PEER_CREATECIVILIAN:
			{
				uint64_t Guid;
				Reader.ReadUInt64(&Guid, 1);

				auto pPed = Strong<CServerHuman>::New(m_pManager->Create(ELEMENT_PED));
				pPed->m_Flags.m_bAlwaysExistForSyncer = true;
				pPed->m_Flags.m_bTransient = true;
				if (!pPed->ReadCreatePacket(pStream) || !pPed->ReadSyncPacket(pStream))
				{
					ReplyWithElementId(Guid, INVALID_NETWORK_ID);
					break;
				}
				if (m_pManager->RegisterNetObject(pPed))
				{
					//_glogprintf(_gstr("PEER2PEER: Civilian %llu assigned to %d"), Guid, pPed->GetId());

					ReplyWithElementId(Guid, pPed->GetId());

					pPed->SetCreatedFor(pClient, true);
					pPed->SetSyncer(pClient, false);
				}
			}
			break;

		case MAFIAPACKET_PEER_CREATEACTOR:
			{
				uint64_t Guid;
				Reader.ReadUInt64(&Guid, 1);

				size_t NameLength = 0;
				AutoFree<GChar> pszName = Reader.ReadString(&NameLength);

				if (pszName == nullptr || NameLength == 0)
					break;

				// Every client independently discovers and reports the same actor (same map, same names for
				// everyone) - FromName is what dedupes them down to a single server-side element instead of
				// one per reporting client.
				auto pExisting = m_pManager->FromName(pszName, ELEMENT_ACTOR);

				int32_t nAssignedId = INVALID_NETWORK_ID;

				if (pExisting != nullptr)
				{
					// This client just lost the race - let it know the id that's already registered.
					// Deliberately does NOT call ReadCreatePacket/ReadSyncPacket here: that would re-run
					// CNetObject::SetName() on the existing object (harmless, same name) but would also
					// stomp its already-correct syncer/dimension with this reporter's possibly-stale view.
					pExisting->SetCreatedFor(pClient, true);
					nAssignedId = pExisting->GetId();
				}
				else
				{
					auto pActor = Strong<CServerActor>::New(m_pManager->Create(ELEMENT_ACTOR));
					pActor->ReadCreatePacket(pStream);
					pActor->ReadSyncPacket(pStream);

					if (m_pManager->RegisterNetObject(pActor))
					{
						pActor->SetCreatedFor(pClient, true);
						pActor->SetSyncer(pClient, false);
						nAssignedId = pActor->GetId();
					}
				}

				Packet Packet(MAFIAPACKET_ELEMENT_UPDATE_ID);
				Packet.Write<uint64_t>(Guid);
				Packet.Write<int32_t>(nAssignedId);
				pClient->SendPacket(&Packet);
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
