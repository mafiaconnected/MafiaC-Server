
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

void CMafiaServer::ProcessPacket(const tPeerInfo& Peer, unsigned int PacketID, Galactic3D::Stream* pStream)
{
	CBinaryReader Reader(pStream);

	CMafiaClient* pClient = static_cast<CMafiaClient*>(m_NetMachines.GetMachineFromPeer(Peer.m_Peer));
	auto pMafiaManager = static_cast<CMafiaServerManager*>(m_pManager);

	{
		CArguments Args(2);
		Args.AddNumber(PacketID);
		Args.AddObject(pClient);

		bool bPreventDefault = false;
		pMafiaManager->m_pOnReceivePacketEventType->Trigger(Args, bPreventDefault);
		if (bPreventDefault)
			return;
	}

	if (PacketID == PACKET_INITIAL)
	{
		// Prevent first packet if first packet already received.
		if (pClient != nullptr)
		{
			// TODO: Disconnect. Because it is likely that trailing data was sent with the PACKET_INITIAL opcode.
			return;
		}

		SendMapName(pClient);
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

					if (bPreventDefault) {
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

			//int32_t nAttackerId;
			//if (!Reader.ReadInt32(&nAttackerId, 1))
			//	return;

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

		case MAFIAPACKET_HUMAN_ENTERINGVEHICLE:
		{
			int32_t nPedId;
			Reader.ReadInt32(&nPedId, 1);

			int32_t nVehicleId;
			Reader.ReadInt32(&nVehicleId, 1);

			int8_t nSeat;
			Reader.ReadInt8(&nSeat, 1);

			int32_t nAction;
			Reader.ReadInt32(&nAction, 1);

			int32_t nUnknown;
			Reader.ReadInt32(&nUnknown, 1);

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
				Packet Packet(MAFIAPACKET_HUMAN_ENTERINGVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				Packet.Write<int32_t>(nAction);
				Packet.Write<int32_t>(nUnknown);
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

			int32_t nAction;
			Reader.ReadInt32(&nAction, 1);

			int32_t nUnknown;
			Reader.ReadInt32(&nUnknown, 1);

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
				Packet Packet(MAFIAPACKET_HUMAN_EXITINGVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				Packet.Write<int32_t>(nAction);
				Packet.Write<int32_t>(nUnknown);
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

			int32_t nAction;
			Reader.ReadInt32(&nAction, 1);

			int32_t nUnknown;
			Reader.ReadInt32(&nUnknown, 1);

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
				Packet Packet(MAFIAPACKET_HUMAN_EXITEDVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				Packet.Write<int32_t>(nAction);
				Packet.Write<int32_t>(nUnknown);
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

			int32_t nAction;
			Reader.ReadInt32(&nAction, 1);

			int32_t nUnknown;
			Reader.ReadInt32(&nUnknown, 1);

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
				Packet Packet(MAFIAPACKET_HUMAN_ENTEREDVEHICLE);
				Packet.Write<int32_t>(pPed->GetId());
				Packet.Write<int32_t>(pVehicle->GetId());
				Packet.Write<int8_t>(nSeat);
				Packet.Write<int32_t>(nAction);
				Packet.Write<int32_t>(nUnknown);
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

		case MAFIAPACKET_VEHICLE_CREATE:
		{
			uint64_t nLocalVehicleId = 0;
			Reader.ReadUInt64(&nLocalVehicleId, 1);

			//CNetObject* pServerElement = m_pManager->FromId(nLocalVehicleId);
			//if (pServerElement == nullptr)

			Strong<CServerVehicle> pServerVehicle;

			{
				pServerVehicle = Strong<CServerVehicle>::New(m_pManager->Create(ELEMENT_VEHICLE));

				if (pServerVehicle == nullptr)
					return;

				pServerVehicle->m_pResource = nullptr;
				pServerVehicle->ReadCreatePacket(pStream);

				//if(!m_pManager->RegisterObject(pServerVehicle))
				//	return;

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

	return true;
}

void CMafiaServer::OnProcess(const FrameTimeInfo* pTime)
{
	CBaseServer::OnProcess(pTime);
}

void CMafiaServer::OnPlayerJoin(CNetMachine* pNetMachine)
{
	CBaseServer::OnPlayerJoin(pNetMachine);
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
		SendEveryonePacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_INFO);
	else
		pClient->SendPacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_INFO);
}

void CMafiaServer::SetMapName(const GChar* pszName)
{
	_gstrlcpy(m_szMap, pszName, ARRAY_COUNT(m_szMap));

	SendMapName(nullptr);
}
