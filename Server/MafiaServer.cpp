
#include "pch.h"
#include "MafiaServer.h"
#include "MafiaServerManager.h"
#include "Elements/Elements.h"

CMafiaServer::CMafiaServer(Galactic3D::Context* pContext) : CBaseServer(pContext)
{
	m_pManager = new CMafiaServerManager(m_pContext, this);

	m_uiNetVersion = NETGAME_CURRENT_VERSION;

	SetRule(_gstr("Version"), __gstr(MAFIAC_SERVER_VERSION));
}

CMafiaServer::~CMafiaServer()
{
}

CNetMachine* CMafiaServer::NewMachine(CServerManager* pServerManager)
{
	return new CMafiaClient(pServerManager);
}

void CMafiaServer::ProcessPacket(Peer_t Peer, unsigned int PacketID, Galactic3D::Stream* pStream)
{
	CBinaryReader Reader(pStream);

	CMafiaClient* pClient = static_cast<CMafiaClient*>(m_NetMachines.GetMachineFromPeer(Peer));
	auto pMafiaManager = static_cast<CMafiaServerManager*>(m_pManager);

	if (PacketID == PACKET_INITIAL)
	{
		// Prevent first packet if first packet already received.
		if (pClient != nullptr)
		{
			// TODO: Disconnect. Because it is likely that trailing data was sent with the PACKET_INITIAL opcode.
			return;
		}
	}
	else
	{
		// Prevent packets unless player actually passed the first packet checks.
		if (pClient == nullptr)
		{
			return;
		}
	}

	pMafiaManager->m_Peer2Peer.ProcessPacket(Peer, PacketID, pStream);

	switch (PacketID)
	{
		case PACKET_RESPONSE:
			{
				SendMapName(pClient);
			}
			break;

		case MAFIAPACKET_HUMAN_SHOOT:
			{
				int32_t nId;
				if (!Reader.ReadInt32(&nId, 1))
					return;

				bool state = false;
				Reader.ReadBoolean(state);

				CVector3D vecShotPosition;
				Reader.ReadVector3D(&vecShotPosition, 1);

				CNetObject* pPed = m_pManager->FromId(nId);
				if (pPed != nullptr && pPed->GetSyncer() == pClient)
				{
					{
						// Scripting event
						CArguments Args(3);
						Args.AddObject(pPed);
						Args.AddBoolean(state);
						Args.AddVector3D(vecShotPosition);
						bool bPreventDefault = false;
						static_cast<CMafiaServerManager*>(m_pManager)->m_pOnPedShootEventType->Trigger(Args, bPreventDefault);

						if (!bPreventDefault)
						{
							Packet Packet(MAFIAPACKET_HUMAN_SHOOT);
							Packet.Write<int32_t>(pPed->GetId());
							Packet.Write<bool>(state);
							Packet.Write<CVector3D>(vecShotPosition);
							m_pManager->SendPacketExcluding(&Packet, pClient);
						}
					}
				}
			}
			break;
		case MAFIAPACKET_HUMAN_THROWGRENADE:
		{
			int32_t nId;
			if (!Reader.ReadInt32(&nId, 1))
				return;

			CVector3D vecShotPosition;
			Reader.ReadVector3D(&vecShotPosition, 1);

			CNetObject* pPed = m_pManager->FromId(nId);
			if (pPed != nullptr && pPed->GetSyncer() == pClient)
			{
				{
					// Scripting event
					CArguments Args(2);
					Args.AddObject(pPed);
					Args.AddVector3D(vecShotPosition);
					bool bPreventDefault = false;
					static_cast<CMafiaServerManager*>(m_pManager)->m_pOnPedThrowGrenadeEventType->Trigger(Args, bPreventDefault);

					if (!bPreventDefault) {
						Packet Packet(MAFIAPACKET_HUMAN_THROWGRENADE);
						Packet.Write<int32_t>(pPed->GetId());
						Packet.Write<CVector3D>(vecShotPosition);
						m_pManager->SendPacketExcluding(&Packet, pClient);
					}
				}
			}
		}
		break;
		case MAFIAPACKET_HUMAN_DROPWEAP:
			{
				int32_t nId;
				if (!Reader.ReadInt32(&nId, 1))
					return;

				CNetObject* pPed = m_pManager->FromId(nId);
				if (pPed != nullptr && pPed->GetSyncer() == pClient)
				{
					{
						Packet Packet(MAFIAPACKET_HUMAN_DROPWEAP);
						Packet.Write<int32_t>(pPed->GetId());
						m_pManager->SendPacketExcluding(&Packet, pClient);
					}
				}
			}
			break;
		case MAFIAPACKET_HUMAN_RELOAD:
		{
			int32_t nId;
			if (!Reader.ReadInt32(&nId, 1))
				return;

			CNetObject* pPed = m_pManager->FromId(nId);
			if (pPed != nullptr && pPed->GetSyncer() == pClient)
			{
				{
					Packet Packet(MAFIAPACKET_HUMAN_RELOAD);
					Packet.Write<int32_t>(pPed->GetId());
					m_pManager->SendPacketExcluding(&Packet, pClient);
				}
			}
		}
		break;
		case MAFIAPACKET_HUMAN_HOLSTER:
		{
			int32_t nId;
			if (!Reader.ReadInt32(&nId, 1))
				return;

			CNetObject* pPed = m_pManager->FromId(nId);
			if (pPed != nullptr && pPed->GetSyncer() == pClient)
			{
				{
					Packet Packet(MAFIAPACKET_HUMAN_HOLSTER);
					Packet.Write<int32_t>(pPed->GetId());
					m_pManager->SendPacketExcluding(&Packet, pClient);
				}
			}
		}
		break;
		case MAFIAPACKET_HUMAN_CHANGEWEAP:
			{
				int32_t nId;
				if (!Reader.ReadInt32(&nId, 1))
					return;

				int32_t nWeapon;
				if (!Reader.ReadInt32(&nWeapon, 1))
					return;

				CNetObject* pPed = m_pManager->FromId(nId);
				if (pPed != nullptr && pPed->GetSyncer() == pClient)
				{
					CServerHuman *pServerHuman = (CServerHuman*)pPed;

					pServerHuman->m_WeaponId = (int16_t)nWeapon;

					{
						Packet Packet(MAFIAPACKET_HUMAN_CHANGEWEAP);
						Packet.Write<int32_t>(pPed->GetId());
						Packet.Write<int32_t>(nWeapon);
						m_pManager->SendPacketExcluding(&Packet, pClient);
					}
				}
			}
			break;
		case MAFIAPACKET_HUMAN_HIT:
		{
			int32_t nTargetId;
			if (!Reader.ReadInt32(&nTargetId, 1))
				return;

			int32_t nAttackerId;
			if (!Reader.ReadInt32(&nAttackerId, 1))
				return;

			CVector3D vecPosition1;
			if (!Reader.ReadVector3D(&vecPosition1, 1))
				return;

			CVector3D vecPosition2;
			if (!Reader.ReadVector3D(&vecPosition2, 1))
				return;

			CVector3D vecPosition3;
			if (!Reader.ReadVector3D(&vecPosition3, 1))
				return;

			int32_t iHitType;
			if (!Reader.ReadInt32(&iHitType, 1))
				return;

			float fDamage;
			if (!Reader.ReadSingle(&fDamage, 1))
				return;

			int32_t iBodyPart;
			if (!Reader.ReadInt32(&iBodyPart, 1))
				return;

			CNetObject* pTargetPed = m_pManager->FromId(nTargetId);
			//CNetObject* pAttackerPed = m_pManager->FromId(nAttackerId);
			if (pTargetPed != nullptr && pTargetPed->GetSyncer() == pClient)
			{
				{
					Packet Packet(MAFIAPACKET_HUMAN_HIT);
					Packet.Write<int32_t>(pTargetPed->GetId());
					//if (pAttackerPed != nullptr) {
					//	Packet.Write<int32_t>(pAttackerPed->GetId());
					//}
					//else {
					//	Packet.Write<int32_t>(INVALID_NETWORK_ID);
					//}
					Packet.Write<CVector3D>(vecPosition1);
					Packet.Write<CVector3D>(vecPosition2);
					Packet.Write<CVector3D>(vecPosition3);
					Packet.Write<int32_t>(iHitType);
					Packet.Write<float>(fDamage);
					Packet.Write<int32_t>(iBodyPart);
					m_pManager->SendPacketExcluding(&Packet, pClient);

					// Scripting event
					CArguments Args(6);
					Args.AddObject(pTargetPed);
					//if (pTargetPed != nullptr) {
					//	Args.AddObject(pAttackerPed);
					//}
					//else {
					//	Args.AddNull();
					//}
					Args.AddVector3D(vecPosition1);
					Args.AddVector3D(vecPosition2);
					Args.AddVector3D(vecPosition3);
					Args.AddNumber(iHitType);
					Args.AddNumber(fDamage);
					Args.AddNumber(iBodyPart);
					bool bPreventDefault = false;
					static_cast<CMafiaServerManager*>(m_pManager)->m_pOnPedHitEventType->Trigger(Args, bPreventDefault);
				}
			}
		}
		break;

		case MAFIAPACKET_HUMAN_DIE:
			{
				int32_t nTargetId;
				if (!Reader.ReadInt32(&nTargetId, 1))
					return;

				int32_t nAttackerId;
				if (!Reader.ReadInt32(&nAttackerId, 1))
					return;

				CNetObject* pTargetPed = m_pManager->FromId(nTargetId);
				CNetObject* pAttackerPed = m_pManager->FromId(nAttackerId);
				if (pTargetPed != nullptr && pTargetPed->GetSyncer() == pClient)
				{
					{
						Packet Packet(MAFIAPACKET_HUMAN_DIE);
						Packet.Write<int32_t>(pTargetPed->GetId());
						if(pAttackerPed != nullptr) {
							Packet.Write<int32_t>(pAttackerPed->GetId());
						} else {
							Packet.Write<int32_t>(INVALID_NETWORK_ID);
						}
						m_pManager->SendPacketExcluding(&Packet, pClient);

						// Scripting event
						CArguments Args(2);
						Args.AddObject(pTargetPed);
						if (pTargetPed != nullptr) {
							Args.AddObject(pAttackerPed);
						}
						else {
							Args.AddNull();
						}
						bool bPreventDefault = false;
						static_cast<CMafiaServerManager*>(m_pManager)->m_pOnPedDeathEventType->Trigger(Args, bPreventDefault);
					}
				}
			}
			break;

		// A ped's syncer asking to enter/exit a vehicle. Its game holds the enter/exit back until we answer, so
		// nothing starts anywhere until this has been checked; the answer (MAFIAPACKET_HUMAN_USEVEHICLE) goes to
		// everyone, the asker included, so all of them start it together. Mafia 2 keeps using the old relays below.
		case MAFIAPACKET_HUMAN_USEVEHICLE_REQUEST:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			int32_t nVehicleId;
			Reader.ReadInt32(&nVehicleId, 1);

			int8_t nDoor;
			Reader.ReadInt8(&nDoor, 1);

			uint32_t nAction;
			Reader.ReadUInt32(&nAction, 1);

			uint32_t nHopSeatsBool;
			Reader.ReadUInt32(&nHopSeatsBool, 1);

			auto pPed = static_cast<CServerHuman*>(m_pManager->FromId(nPedId, ELEMENT_PED));
			auto pVehicle = static_cast<CServerVehicle*>(m_pManager->FromId(nVehicleId, ELEMENT_VEHICLE));

			if (pPed == nullptr
				|| pVehicle == nullptr
				|| pClient != pPed->GetSyncer()
				|| nDoor < 0
				|| nDoor > 20)
			{
				_glogprintf(_gstr("USEVEHICLE_REQUEST rejected: bad request (ped %d %s, vehicle %d %s, from the ped's syncer: %d, door %d)"), nPedId, pPed != nullptr ? _gstr("found") : _gstr("missing"), nVehicleId, pVehicle != nullptr ? _gstr("found") : _gstr("missing"), (pPed != nullptr && pClient == pPed->GetSyncer()) ? 1 : 0, (int)nDoor);
				break;
			}

			const bool bExit = (nAction == 2);

			// Going in by the passenger door with hop-seats set ends up in the driver's seat. Leaving is from the seat
			// the ped is recorded in, whatever the game passed.
			int8_t nSeat = (nHopSeatsBool == 1 && nDoor == 1) ? 0 : nDoor;
			if (bExit && pPed->m_nVehicleNetworkIndex == pVehicle->GetId() && pPed->m_nSeat >= 0)
				nSeat = pPed->m_nSeat;

			if (!bExit && (nSeat < 0 || nSeat >= ARRAY_COUNT(CServerVehicle::m_pProbableOccupants)))
			{
				_glogprintf(_gstr("USEVEHICLE_REQUEST rejected: ped %d, vehicle %d, seat %d out of range (door %d, hop-seats %u)"), pPed->GetId(), pVehicle->GetId(), (int)nSeat, (int)nDoor, nHopSeatsBool);
				break;
			}

			// A locked vehicle can't be gone into
			if (!bExit && pVehicle->GetLocked())
			{
				_glogprintf(_gstr("USEVEHICLE_REQUEST rejected: ped %d, vehicle %d is locked"), pPed->GetId(), pVehicle->GetId());
				break;
			}

			{
				CArguments Args(3);
				Args.AddObject(pPed);
				Args.AddObject(pVehicle);
				Args.AddNumber(nSeat);
				bool bPreventDefault = false;
				(bExit ? pMafiaManager->m_pOnPedExitingVehicleEventType : pMafiaManager->m_pOnPedEnteringVehicleEventType)->Trigger(Args, bPreventDefault);
				if (bPreventDefault)
				{
					_glogprintf(_gstr("USEVEHICLE_REQUEST rejected: ped %d, vehicle %d, seat %d vetoed by a script"), pPed->GetId(), pVehicle->GetId(), (int)nSeat);
					break;
				}
			}

			_glogprintf(_gstr("USEVEHICLE_REQUEST approved: ped %d %s vehicle %d (door %d, seat %d)"), pPed->GetId(), bExit ? _gstr("exits") : _gstr("enters"), pVehicle->GetId(), (int)nDoor, (int)nSeat);

			if (bExit)
				pPed->LeaveVehicleSeat();
			else
				pPed->EnterVehicleSeat(pVehicle, nSeat);

			{
				Packet Packet(MAFIAPACKET_HUMAN_USEVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nDoor);
				Packet.Write<int8_t>(nSeat);
				Packet.Write<uint32_t>(nAction);
				Packet.Write<uint32_t>(nHopSeatsBool);
				m_pManager->SendPacketExcluding(&Packet, nullptr);
			}

			if (!bExit && nSeat == 0 && pVehicle->CanBeSyncer(pClient))
			{
				_glogprintf(_gstr("Setting vehicle %d syncer to %d"), pVehicle->GetId(), pClient->m_nIndex);
				pVehicle->SetSyncer(pClient, true);
			}
		}
		break;

		case MAFIAPACKET_HUMAN_ENTERINGVEHICLE:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			int32_t nVehicleId;
			Reader.ReadInt32(&nVehicleId, 1);

			int8_t nSeat;
			Reader.ReadInt8(&nSeat, 1);

			uint32_t nAction;
			Reader.ReadUInt32(&nAction, 1);

			uint32_t nHopSeatsBool;
			Reader.ReadUInt32(&nHopSeatsBool, 1);

			CServerHuman* pPed = (CServerHuman*)m_pManager->FromId(nPedId);
			CNetObject* pVehicle = m_pManager->FromId(nVehicleId);

			if (pPed == nullptr
			|| pVehicle == nullptr
			|| pClient != pPed->GetSyncer()
			|| nSeat < 0
			|| nSeat > 20)
			{
				break;
			}

			pPed->m_EnteringExitingVehicle = true;

			{
				Packet Packet(MAFIAPACKET_HUMAN_ENTERINGVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				Packet.Write<uint32_t>(nAction);
				Packet.Write<uint32_t>(nHopSeatsBool);
				m_pManager->SendPacketExcluding(&Packet, pClient);
			}

			if (nSeat == 0 && pVehicle->CanBeSyncer(pClient))
			{
				_glogprintf(_gstr("Setting vehicle %d syncer to %d"), pVehicle->GetId(), pClient->m_nIndex);
				pVehicle->SetSyncer(pClient, true);
			}
		}
		break;

		case MAFIAPACKET_HUMAN_EXITINGVEHICLE:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			int32_t nVehicleId;
			Reader.ReadInt32(&nVehicleId, 1);

			int8_t nSeat;
			Reader.ReadInt8(&nSeat, 1);

			uint32_t nAction;
			Reader.ReadUInt32(&nAction, 1);

			uint32_t nUnknown;
			Reader.ReadUInt32(&nUnknown, 1);

			CServerHuman* pPed = (CServerHuman*)m_pManager->FromId(nPedId);
			CNetObject* pVehicle = m_pManager->FromId(nVehicleId);

			if (pPed == nullptr
				|| pVehicle == nullptr
				|| pClient != pPed->GetSyncer()
				|| nSeat < 0
				|| nSeat > 20)
			{
				break;
			}

			pPed->m_EnteringExitingVehicle = true;

			{
				Packet Packet(MAFIAPACKET_HUMAN_EXITINGVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				Packet.Write<uint32_t>(nAction);
				Packet.Write<uint32_t>(nUnknown);
				m_pManager->SendPacketExcluding(&Packet, pClient);
			}
		}
		break;

		case MAFIAPACKET_HUMAN_EXITEDVEHICLE:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			int32_t nVehicleId;
			Reader.ReadInt32(&nVehicleId, 1);

			int8_t nSeat;
			Reader.ReadInt8(&nSeat, 1);

			CServerHuman* pPed = (CServerHuman*)m_pManager->FromId(nPedId);
			CNetObject* pVehicle = m_pManager->FromId(nVehicleId);

			if (pPed == nullptr
				|| pVehicle == nullptr
				|| pClient != pPed->GetSyncer()
				|| nSeat < 0
				|| nSeat > 20)
			{
				break;
			}

			pPed->m_EnteringExitingVehicle = false;

			{
				Packet Packet(MAFIAPACKET_HUMAN_EXITEDVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				m_pManager->SendPacketExcluding(&Packet, pClient);
			}
		}
		break;

		case MAFIAPACKET_HUMAN_ENTEREDVEHICLE:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			int32_t nVehicleId;
			Reader.ReadInt32(&nVehicleId, 1);

			int8_t nSeat;
			Reader.ReadInt8(&nSeat, 1);

			CServerHuman* pPed = (CServerHuman*)m_pManager->FromId(nPedId);
			CNetObject* pVehicle = m_pManager->FromId(nVehicleId);

			if (pPed == nullptr
				|| pVehicle == nullptr
				|| pClient != pPed->GetSyncer()
				|| nSeat < 0
				|| nSeat > 20)
			{
				break;
			}

			pPed->m_EnteringExitingVehicle = false;

			{
				Packet Packet(MAFIAPACKET_HUMAN_ENTEREDVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				m_pManager->SendPacketExcluding(&Packet, pClient);
			}

			if (nSeat == 0 && pVehicle->CanBeSyncer(pClient))
			{
				_glogprintf(_gstr("Setting vehicle %d syncer to %d"), pVehicle->GetId(), pClient->m_nIndex);
				pVehicle->SetSyncer(pClient, true);
			}
		}
		break;

		case MAFIAPACKET_HUMAN_JACKVEHICLE:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			int32_t nVehicleId;
			Reader.ReadInt32(&nVehicleId, 1);

			int8_t nSeat;
			Reader.ReadInt8(&nSeat, 1);

			CNetObject* pPed = m_pManager->FromId(nPedId);
			CNetObject* pVehicle = m_pManager->FromId(nVehicleId);

			if (pPed == nullptr
				|| pVehicle == nullptr
				|| pClient != pPed->GetSyncer()
				|| nSeat < 0
				|| nSeat > 20)
			{
				break;
			}

			{
				Packet Packet(MAFIAPACKET_HUMAN_JACKVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				m_pManager->SendPacketExcluding(&Packet, pClient);
			}

			if (nSeat == 0)
			{
				pVehicle->SetSyncer(pClient);
			}
		}
		break;

		case MAFIAPACKET_HUMAN_USINGACTOR:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			size_t size = 0;
			GChar* szName = Reader.ReadString(&size);
			 
			uint32_t iUnknown1;
			Reader.ReadUInt32(&iUnknown1, 1);

			uint32_t iUnknown2;
			Reader.ReadUInt32(&iUnknown2, 1);

			uint32_t iUnknown3;
			Reader.ReadUInt32(&iUnknown3, 1);

			CNetObject* pPed = m_pManager->FromId(nPedId);

			if (pPed == nullptr || pClient != pPed->GetSyncer())
			{
				break;
			}

			_glogprintf(_gstr("[CMafiaServer::ProcessPacket] (MAFIAPACKET_HUMAN_USINGACTOR) Human: %d, Actor: %s, iUnknown1: %d, iUnknown2: %d, iUnknown3: %d"), pPed->GetId(), szName, iUnknown1, iUnknown2, iUnknown3);

			// Scripting event
			CArguments Args(5);
			Args.AddObject(pPed);
			Args.AddString(szName);
			Args.AddNumber(iUnknown1);
			Args.AddNumber(iUnknown2);
			Args.AddNumber(iUnknown3);
			bool bPreventDefault = false;
			static_cast<CMafiaServerManager*>(m_pManager)->m_pOnPedUseActorEventType->Trigger(Args, bPreventDefault);

			if (!bPreventDefault) {
				Packet Packet(MAFIAPACKET_HUMAN_USINGACTOR);
				Packet.Write<int32_t>(pPed->GetId());
				CBinaryWriter Writer(&Packet);
				Writer.WriteString(szName);
				Packet.Write<uint32_t>(nUnk1);
				Packet.Write<uint32_t>(nUnk2);
				Packet.Write<uint32_t>(nUnk3);
				m_pManager->SendPacketExcluding(&Packet, pClient);
			}
		}
		break;

		case MAFIAPACKET_VEHICLE_CREATE:
		{
			uint64_t nLocalVehicleId = 0;
			Reader.ReadUInt64(&nLocalVehicleId, 1);

			Strong<CServerVehicle> pServerVehicle;

			{
				pServerVehicle = Strong<CServerVehicle>::New(m_pManager->Create(ELEMENT_VEHICLE));

				if (pServerVehicle == nullptr)
					return;

				pServerVehicle->m_pResource = nullptr;
				pServerVehicle->ReadCreatePacket(pStream);

				pServerVehicle->SetCreatedFor(pClient, true);
				pServerVehicle->SetSyncer(pClient, true);
			}

			{
				Packet Packet(MAFIAPACKET_ELEMENT_UPDATE_ID);
				Packet.Write<uint64_t>(nLocalVehicleId);
				Packet.Write<int32_t>(pServerVehicle->GetId());
				pClient->SendPacket(&Packet);
			}
		}
		break;

		/*
		case MAFIAPACKET_ELEMENT_REMOVE:
		{
			bool bRemoveByElementId;
			Reader.ReadBoolean(bRemoveByElementId);

			if (bRemoveByElementId)
			{
				uint32_t nServerVehicleId = 0;
				Reader.ReadUInt32(&nServerVehicleId, 1);

				CServerVehicle* pServerVehicle = (CServerVehicle*)m_pManager->FromId(nServerVehicleId);

				if (pServerVehicle != nullptr)
				{
					printf("Destroyed vehicle ID %i\n", pServerVehicle->GetId());

					{
						Packet Packet(MAFIAPACKET_ELEMENT_REMOVE);
						Packet.Write<int32_t>(pServerVehicle->GetId());
						m_pManager->SendPacketExcluding(&Packet, pClient);
					}

					m_pManager->Remove(pServerVehicle);
				}
			}
			else
			{
				uint64_t nServerVehicleGuid = 0;
				Reader.ReadUInt64(&nServerVehicleGuid, 1);
			}
		}
		break;
		*/
	}

	CBaseServer::ProcessPacket(Peer, PacketID, pStream);
}

bool CMafiaServer::ParseConfig(const CServerConfiguration& Config)
{
	if (!CBaseServer::ParseConfig(Config))
		return false;

	_gstrlcpy(m_szMap, Config.GetStringValue(_gstr("mapname"), _gstr("")), ARRAY_COUNT(m_szMap));

	return true;
}

void CMafiaServer::OnProcess(const FrameTimeInfo* pTime)
{
	CBaseServer::OnProcess(pTime);
}

void CMafiaServer::OnPlayerJoin(CNetMachine* pNetMachine)
{
	CBaseServer::OnPlayerJoin(pNetMachine);
	SendMapName(pNetMachine);
}

void CMafiaServer::OnPlayerJoined(CNetMachine* pNetMachine)
{
	CBaseServer::OnPlayerJoined(pNetMachine);
}

void CMafiaServer::SendMapName(CNetMachine* pClient)
{
	Packet Packet(MAFIAPACKET_CHANGEMAP);
	CBinaryWriter Writer(&Packet);
	Writer.WriteString(m_szMap);

	if (pClient == nullptr)
		SendEveryonePacket(&Packet);
	else
		pClient->SendPacket(&Packet);
}

void CMafiaServer::SetMapName(const GChar* pszName)
{
	_gstrlcpy(m_szMap, pszName, ARRAY_COUNT(m_szMap));

	SendMapName(nullptr);
}
