
#include "pch.h"
#include "MafiaServer.h"
#include "MafiaServerManager.h"
#include "ServerHuman.h"
#include "ServerVehicle.h"

CServerHuman::CServerHuman(CMafiaServerManager* pServerManager) : CServerEntity(pServerManager)
{
	m_Type = ELEMENT_PED;

	m_Camera = CVector3D(0.0f, 0.0f, 0.0f);

	m_AnimationStateLocal = 0;
	m_IsInAnimWithCarLocal = false;
	m_AnimationState = 0;
	m_IsInAnimWithCar = false;
	m_fInCarRotation = 0.0f;
	m_IsCrouching = false;
	m_IsAiming = false;
	m_IsShooting = false;
	m_iAnimStopTime = 0;
	m_WeaponId = 0;

	m_fCurrentRotation = 0.0;
	m_fHealth = 200;
	m_nVehicleNetworkIndex = INVALID_NETWORK_ID;
	m_nSeat = 0;
	m_Behavior = 4;

	m_EnteringExitingVehicle = false;
}

void CServerHuman::SetModel(const GChar* sModel)
{
	CServerEntity::SetModel(sModel);

	if (GetId() != INVALID_NETWORK_ID)
	{
		Packet Packet(MAFIAPACKET_HUMAN_SETMODEL);
		CBinaryWriter Writer(&Packet);
		Writer.WriteInt32(GetId());
		Writer.WriteString(sModel);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

ReflectedClass* CServerHuman::GetReflectedClass()
{
	return static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pServerHumanClass;
}

void CServerHuman::OnCreated(void)
{
	CServerEntity::OnCreated();
}

void CServerHuman::OnSpawned()
{
	CArguments Arguments(1);
	Arguments.AddObject(this);
	static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pOnPedSpawnEventType->Trigger(Arguments);
}

bool CServerHuman::CanExistForMachine(CNetMachine* pClient)
{
	if (m_nVehicleNetworkIndex != INVALID_NETWORK_ID && m_pNetObjectMgr->DoesIdExist(m_nVehicleNetworkIndex))
	{
		auto pVehicle = m_pNetObjectMgr->FromId(m_nVehicleNetworkIndex);
		if (pVehicle->IsCreatedFor(pClient))
			return true;
		return false;
	}
	return CServerEntity::CanExistForMachine(pClient);
}

bool CServerHuman::ReadCreatePacket(Stream* pStream)
{
	if (!CServerEntity::ReadCreatePacket(pStream))
		return false;

	tHumanCreatePacket Packet;

	if (pStream->Read(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	m_fHealth = Packet.health;
	m_nVehicleNetworkIndex = Packet.vehicleNetworkIndex;
	m_nSeat = Packet.seat;
	m_IsCrouching = Packet.isCrouching;
	m_IsAiming = Packet.isAiming;
	m_IsShooting = Packet.isShooting;
	m_AnimationStateLocal = Packet.animStateLocal;
	m_IsInAnimWithCarLocal = Packet.isInAnimWithCarLocal;
	m_AnimationState = Packet.animationState;
	m_IsInAnimWithCar = Packet.isInAnimWithCar;
	m_iAnimStopTime = Packet.animStopTime;
	m_WeaponId = Packet.weaponId;

	m_RelPosition = { 0,0,0 };
	m_RelRotation = { 0,0,0 };

	return true;
}

bool CServerHuman::ReadSyncPacket(Stream* pStream)
{
	if (!CServerEntity::ReadSyncPacket(pStream))
		return false;

	tHumanSyncPacket Packet;

	if (pStream->Read(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	int32_t nOldVehicleNetworkIndex = m_nVehicleNetworkIndex;
	int8_t nOldSeat = m_nSeat;

	m_fHealth = Packet.health;
	m_nVehicleNetworkIndex = Packet.vehicleNetworkIndex;
	m_nSeat = Packet.seat;
	m_IsCrouching = Packet.isCrouching;
	m_IsAiming = Packet.isAiming;
	m_IsShooting = Packet.isShooting;
	m_AnimationStateLocal = Packet.animStateLocal;
	m_IsInAnimWithCarLocal = Packet.isInAnimWithCarLocal;
	m_AnimationState = Packet.animationState;
	m_IsInAnimWithCar = Packet.isInAnimWithCar;
	m_fInCarRotation = Packet.inCarRotation;
	m_iAnimStopTime = Packet.animStopTime;
	m_WeaponId = Packet.weaponId;
	m_Camera = Packet.camera;

	if (m_nVehicleNetworkIndex == INVALID_NETWORK_ID)
	{
		auto pOldVehicle = static_cast<CServerVehicle*>(m_pNetObjectMgr->FromId(m_nVehicleNetworkIndex, ELEMENT_VEHICLE));
		if (pOldVehicle != nullptr)
		{
			if (nOldSeat > 0 && nOldSeat < ARRAY_COUNT(CServerVehicle::m_pProbableOccupants))
			{
				pOldVehicle->m_pProbableOccupants[nOldSeat] = nullptr;
			}
		}
	}
	else
	{
		auto pOldVehicle = static_cast<CServerVehicle*>(m_pNetObjectMgr->FromId(nOldVehicleNetworkIndex, ELEMENT_VEHICLE));
		auto pNewVehicle = static_cast<CServerVehicle*>(m_pNetObjectMgr->FromId(m_nVehicleNetworkIndex, ELEMENT_VEHICLE));

		if (pNewVehicle != nullptr)
		{
			if (pOldVehicle == nullptr)
			{
				if (m_nSeat > 0 && m_nSeat < ARRAY_COUNT(CServerVehicle::m_pProbableOccupants))
				{
					pNewVehicle->m_pProbableOccupants[m_nSeat] = this;
				}
			}
			else if (pOldVehicle != pNewVehicle)
			{
				if (nOldSeat > 0 && nOldSeat < ARRAY_COUNT(CServerVehicle::m_pProbableOccupants))
				{
					pOldVehicle->m_pProbableOccupants[nOldSeat] = nullptr;
				}

				if (m_nSeat > 0 && m_nSeat < ARRAY_COUNT(CServerVehicle::m_pProbableOccupants))
				{
					pNewVehicle->m_pProbableOccupants[m_nSeat] = this;
				}
			}

			CVector3D vecNewVehiclePos;
			if (pNewVehicle->GetPosition(vecNewVehiclePos))
			{
				SetPosition(vecNewVehiclePos);
			}
		}
	}

	//GChar szHost[256];
	//GetSyncer()->m_IPAddress.ToString(szHost, ARRAY_SIZE(szHost));
	//_glogprintf(L"Got sync packet for human #%d (%s - ip %s):\n\tPosition: [%f, %f, %f]\n\tPos. difference: [%f, %f, %f]\n\tRotation: [%f, %f, %f]\n\tRot. difference: [%f, %f, %f]\n\tHealth: %f\n\tVehicle index: %d\n\tDucking: %s\n\tAiming: %s\n\tAnim state: %d", GetId(), machine->GetName(), szHost, m_Position.x, m_Position.y, m_Position.z, m_RelPosition.x, m_RelPosition.y, m_RelPosition.z, m_Rotation.x, m_Rotation.y, m_Rotation.z, m_RelRotation.x, m_RelRotation.y, m_RelRotation.z, m_fHealth, m_nVehicleNetworkIndex, m_IsCrouching ? L"Yes" : L"No", m_IsAiming ? L"Yes" : L"No", m_AnimationState);

	return true;
}

bool CServerHuman::WriteCreatePacket(Stream* pStream)
{
	if (!CServerEntity::WriteCreatePacket(pStream))
		return false;

	tHumanCreatePacket Packet;

	Packet.health = m_fHealth;
	Packet.vehicleNetworkIndex = m_nVehicleNetworkIndex;
	Packet.seat = m_nSeat;
	Packet.isCrouching = m_IsCrouching;
	Packet.isAiming = m_IsAiming;
	Packet.isShooting = m_IsShooting;
	Packet.animStateLocal = m_AnimationStateLocal;
	Packet.isInAnimWithCarLocal = m_IsInAnimWithCarLocal;
	Packet.animationState = m_AnimationState;
	Packet.isInAnimWithCar = m_IsInAnimWithCar;
	Packet.animStopTime = m_iAnimStopTime;
	Packet.weaponId = m_WeaponId;

	if (pStream->Write(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	return true;
}

bool CServerHuman::WriteSyncPacket(Stream* pStream)
{
	if (!CServerEntity::WriteSyncPacket(pStream))
		return false;

	tHumanSyncPacket Packet;

	Packet.health = m_fHealth;
	Packet.vehicleNetworkIndex = m_nVehicleNetworkIndex;
	Packet.seat = m_nSeat;
	Packet.isCrouching = m_IsCrouching;
	Packet.isAiming = m_IsAiming;
	Packet.isShooting = m_IsShooting;
	Packet.animStateLocal = m_AnimationStateLocal;
	Packet.isInAnimWithCarLocal = m_IsInAnimWithCarLocal;
	Packet.animationState = m_AnimationState;
	Packet.isInAnimWithCar = m_IsInAnimWithCar;
	Packet.inCarRotation = m_fInCarRotation;
	Packet.animStopTime = m_iAnimStopTime;
	Packet.weaponId = m_WeaponId;
	Packet.camera = m_Camera;

	if (pStream->Write(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	//auto machine = GetSyncer();
	//GChar szHost[256];
	//machine->m_IPAddress.ToString(szHost, ARRAY_SIZE(szHost));
	//_glogprintf(L"Sent sync packet for element #%d (%s - ip %s):\n\tPosition: [%f, %f, %f]\n\tPos. difference: [%f, %f, %f]\n\tRotation: [%f, %f, %f]\n\tRot. difference: [%f, %f, %f]\n\tHealth: %f\n\tVehicle index: %d\n\tDucking: %s\n\tAiming: %s\n\tAnim state: %d", GetId(), machine->GetName(), szHost, m_Position.x, m_Position.y, m_Position.z, m_RelPosition.x, m_RelPosition.y, m_RelPosition.z, m_Rotation.x, m_Rotation.y, m_Rotation.z, m_RelRotation.x, m_RelRotation.y, m_RelRotation.z, m_fHealth, m_nVehicleNetworkIndex, m_IsCrouching ? L"Yes" : L"No", m_IsAiming ? L"Yes" : L"No", m_AnimationState);

	return true;
}

void CServerHuman::Shoot(bool state, const CVector3D& distPos)
{
	Packet Packet(MAFIAPACKET_HUMAN_SHOOT);
	Packet.Write<int32_t>(GetId());
	Packet.Write<bool>(state);
	Packet.Write<CVector3D>(distPos);
	m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
}

void CServerHuman::SetHealth(float fHealth)
{
	Packet Packet(MAFIAPACKET_HUMAN_SETHEALTH);
	Packet.Write<int32_t>(GetId());
	Packet.Write<float>(fHealth);
	m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
}

void CServerHuman::Jump()
{
	Packet Packet(MAFIAPACKET_HUMAN_JUMP);
	Packet.Write<int32_t>(GetId());
	m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
}

void CServerHuman::ThrowGrenade(const CVector3D& dstPos)
{
	Packet Packet(MAFIAPACKET_HUMAN_THROWGRENADE);
	Packet.Write<int32_t>(GetId());
	Packet.Write<CVector3D>(dstPos);
	m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
}

void CServerHuman::Reload()
{
	Packet Packet(MAFIAPACKET_HUMAN_RELOAD);
	Packet.Write<int32_t>(GetId());
	m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
}

void CServerHuman::HolsterWeapon()
{
	Packet Packet(MAFIAPACKET_HUMAN_HOLSTER);
	Packet.Write<int32_t>(GetId());
	m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
}

void CServerHuman::ClearInventory()
{
	for (int i = 0; i < 8; i++)
	{
		TakeWeapon(items[i].weapId);
	}
}

void CServerHuman::GiveWeapon(unsigned short ucWeapon, unsigned short ucAmmo1, unsigned short ucAmmo2)
{
	if (!HasWeapon(ucWeapon))
	{
		auto index = GetFirstEmptyWeaponIndex();

		if (index < 8)
		{
			InventoryItem item = items[index];

			item.weapId = ucWeapon;
			item.ammo1 = ucAmmo1;
			item.ammo2 = ucAmmo2;

			Packet Packet(MAFIAPACKET_HUMAN_ADDWEAP);
			Packet.Write<int32_t>(GetId());
			Packet.Write<unsigned short>(ucWeapon);
			Packet.Write<unsigned short>(ucAmmo1);
			Packet.Write<unsigned short>(ucAmmo2);
			m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
		}
	}
}

bool CServerHuman::HasWeapon(unsigned short ucWeapon)
{
	for (int i = 0; i < 8; i++)
	{
		InventoryItem item = items[i];

		if (item.weapId == ucWeapon) return true;
	}

	return false;
}

int CServerHuman::GetFirstEmptyWeaponIndex()
{
	for (int i = 0; i < 8; i++)
	{
		InventoryItem item = items[i];

		if (item.weapId == 0) return i;
	}

	return -1;
}

int CServerHuman::GetIndexOfWeapon(unsigned short ucWeapon)
{
	for (int i = 0; i < 8; i++)
	{
		InventoryItem item = items[i];

		if (item.weapId == ucWeapon) return i;
	}

	return -1;
}

void CServerHuman::TakeWeapon(unsigned short ucWeapon)
{
	if (HasWeapon(ucWeapon))
	{
		auto index = GetIndexOfWeapon(ucWeapon);

		InventoryItem item = items[index];

		item.weapId = 0;
		item.ammo1 = 0;
		item.ammo2 = 0;

		Packet Packet(MAFIAPACKET_HUMAN_REMWEAP);
		Packet.Write<int32_t>(GetId());
		Packet.Write<unsigned short>(ucWeapon);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

void CServerHuman::DropWeapon()
{
	Packet Packet(MAFIAPACKET_HUMAN_DROPWEAP);
	Packet.Write<int32_t>(GetId());
	m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
}

void CServerHuman::Kill()
{
	Packet Packet(MAFIAPACKET_HUMAN_DIE);
	Packet.Write<int32_t>(GetId());
	m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);

	CArguments Arguments(3);
	Arguments.AddObject(this); // The killed PED
	Arguments.AddNumber(10); // The death reason (how he died)
	Arguments.AddObject(this); // The killer PED
	Arguments.AddNumber(0); // The hit part (head, torso, etc)
	static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pOnPedDeathEventType->Trigger(Arguments);
}

bool CServerHuman::IsInVehicle(void)
{
	return m_nVehicleNetworkIndex != INVALID_NETWORK_ID;
}

bool CServerHuman::CanEnterVehicle(CServerVehicle* pVehicle, int8_t iSeat)
{
	//CArguments Arguments(3);
	//Arguments.AddObject(this);
	//Arguments.AddObject(pVehicle);
	//Arguments.AddNumber(ucSeat);
	//bool bPreventDefault = false;
	//static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pOnPedEnteredVehicleEventType->Trigger(Arguments, bPreventDefault);
	//if (bPreventDefault)
	//	return false;

	return true;
}

bool CServerHuman::CanExitVehicle(void)
{
	if (m_nVehicleNetworkIndex != INVALID_NETWORK_ID)
	{
		//auto pVehicle = static_cast<CServerVehicle*>(m_pNetObjectMgr->FromId(m_nVehicleNetworkIndex));
		//
		//{
		//	CArguments Arguments(2);
		//	Arguments.AddObject(this);
		//	Arguments.AddObject(pVehicle);
		//	bool bPreventDefault = false;
		//	static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pOnPedExitVehicleEventType->Trigger(Arguments,bPreventDefault);
		//	if (bPreventDefault)
		//		return false;
		//}

		return true;
	}

	return false;
}

void CServerHuman::WarpIntoVehicle(CServerVehicle* pVehicle, int8_t iSeat)
{

}
