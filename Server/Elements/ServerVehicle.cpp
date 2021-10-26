
#include "pch.h"
#include "Server.h"
#include "MafiaServerManager.h"
#include <Utils/VectorTools.h>

CServerVehicle::CServerVehicle(CMafiaServerManager* pServerManager) : CServerEntity(pServerManager)
{
	m_Type = ELEMENT_VEHICLE;

	CMatrix3x3 mat3(CVector3D(0, 0, 0));
	m_RotationFront = mat3.GetXAxis();
	m_RotationUp = mat3.GetYAxis();
	m_RotationRight = mat3.GetZAxis();
}

ReflectedClass* CServerVehicle::GetReflectedClass(void)
{
	return static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pServerVehicleClass;
}

bool CServerVehicle::ShouldExistForMachine(CNetMachine* pClient)
{
	auto pPlayer = pClient->GetPlayer();
	if (pPlayer != NULL && pPlayer->IsType(ELEMENT_PED))
	{
		CServerHuman* pHuman = static_cast<CServerHuman*>(pPlayer);
		if (pHuman->m_nVehicleNetworkIndex == GetId()) // if client is in this vehicle
		{
			return true;
		}
	}
	return CServerEntity::ShouldExistForMachine(pClient);
}

bool CServerVehicle::ShouldDeleteForMachine(CNetMachine* pClient)
{
	auto pPlayer = pClient->GetPlayer();
	if (pPlayer != NULL && pPlayer->IsType(ELEMENT_PED))
	{
		CServerHuman* pHuman = static_cast<CServerHuman*>(pPlayer);
		if (pHuman->m_nVehicleNetworkIndex == GetId()) // if client is in this vehicle
		{
			return false;
		}
	}
	return CServerEntity::ShouldDeleteForMachine(pClient);
}

bool CServerVehicle::ReadCreatePacket(Stream* pStream)
{
	if (!CServerEntity::ReadCreatePacket(pStream))
		return false;

	m_RelPosition = { 0,0,0 };
	m_RelRotation = { 0,0,0 };

	tVehicleCreatePacket Packet;

	if (pStream->Read(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	m_RotationFront = Packet.rotationFront;
	m_RotationUp = Packet.rotationUp;
	m_RotationRight = Packet.rotationRight;
	m_Health = Packet.health;
	m_EngineHealth = Packet.engineHealth;
	m_Fuel = Packet.fuel;
	m_SoundEnabled = Packet.sound;
	m_Engine = Packet.engineOn;
	m_Horn = Packet.horn;
	m_Siren = Packet.siren;
	m_Lights = Packet.lights;
	m_Gear = Packet.gear;
	m_EngineRPM = Packet.rpm;
	m_Accelerating = Packet.accel;
	m_Breakval = Packet.brake;
	m_Handbrake = Packet.handBrake;
	m_SpeedLimit = Packet.speedLimit;
	m_Clutch = Packet.clutch;
	m_WheelAngle = Packet.wheelAngle;
	m_Velocity = Packet.speed;
	m_RotVelocity = Packet.rotSpeed;

	auto machine = m_pServerManager->m_pNetMachines->GetMachine(GetSyncer());
	GChar szHost[256];
	machine->m_IPAddress.ToString(szHost, ARRAY_SIZE(szHost));
	//_glogprintf(_gstr("Got create packet for vehicle #%d (%s - ip %s):\n\tPosition: [%f, %f, %f]\n\tPos. difference: [%f, %f, %f]\n\tRotation: [%f, %f, %f]\n\tRot. difference: [%f, %f, %f]\n\tHealth: %f\n"), GetId(), machine->GetName(), szHost, m_Position.x, m_Position.y, m_Position.z, m_RelPosition.x, m_RelPosition.y, m_RelPosition.z, m_Rotation.x, m_Rotation.y, m_Rotation.z, m_RelRotation.x, m_RelRotation.y, m_RelRotation.z);

	return true;
}

bool CServerVehicle::ReadSyncPacket(Stream* pStream)
{
	if (!CServerEntity::ReadSyncPacket(pStream))
		return false;

	tVehicleSyncPacket Packet;

	if (pStream->Read(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	m_RotationFront = Packet.rotationFront;
	m_RotationUp = Packet.rotationUp;
	m_RotationRight = Packet.rotationRight;
	m_Health = Packet.health;
	m_EngineHealth = Packet.engineHealth;
	m_Fuel = Packet.fuel;
	m_SoundEnabled = Packet.sound;
	m_Engine = Packet.engineOn;
	m_Horn = Packet.horn;
	m_Siren = Packet.siren;
	m_Lights = Packet.lights;
	m_Gear = Packet.gear;
	m_EngineRPM = Packet.rpm;
	m_Accelerating = Packet.accel;
	m_Breakval = Packet.brake;
	m_Handbrake = Packet.handBrake;
	m_SpeedLimit = Packet.speedLimit;
	m_Clutch = Packet.clutch;
	m_WheelAngle = Packet.wheelAngle;
	m_Velocity = Packet.speed;
	m_RotVelocity = Packet.rotSpeed;

	auto machine = m_pServerManager->m_pNetMachines->GetMachine(GetSyncer());
	GChar szHost[256];
	machine->m_IPAddress.ToString(szHost, ARRAY_SIZE(szHost));
	_glogprintf(_gstr("Got sync packet for vehicle #%d (%s - ip %s):\n\tPosition: [%f, %f, %f]\n\tPos. difference: [%f, %f, %f]\n\tRotation: [%f, %f, %f]\n\tRot. difference: [%f, %f, %f]\n\tHealth: %f\n"), GetId(), machine->GetName(), szHost, m_Position.x, m_Position.y, m_Position.z, m_RelPosition.x, m_RelPosition.y, m_RelPosition.z, m_Rotation.x, m_Rotation.y, m_Rotation.z, m_RelRotation.x, m_RelRotation.y, m_RelRotation.z);

	return true;
}

bool CServerVehicle::WriteCreatePacket(Stream* pStream)
{
	if (!CServerEntity::WriteCreatePacket(pStream))
		return false;

	tVehicleCreatePacket Packet;

	Packet.rotationFront = m_RotationFront;
	Packet.rotationUp = m_RotationUp;
	Packet.rotationRight = m_RotationRight;
	Packet.health = m_Health;
	Packet.engineHealth = m_EngineHealth;
	Packet.fuel = m_Fuel;
	Packet.sound = m_SoundEnabled;
	Packet.engineOn = m_Engine;
	Packet.horn = m_Horn;
	Packet.siren = m_Siren;
	Packet.lights = m_Lights;
	Packet.gear = m_Gear;
	Packet.rpm = m_EngineRPM;
	Packet.accel = m_Accelerating;
	Packet.brake = m_Breakval;
	Packet.handBrake = m_Handbrake;
	Packet.speedLimit = m_SpeedLimit;
	Packet.clutch = m_Clutch;
	Packet.wheelAngle = m_WheelAngle;
	Packet.speed = m_Velocity;
	Packet.rotSpeed = m_RotVelocity;

	//_glogprintf(_gstr("Sent create packet for vehicle #%d:\n\tPosition: [%f, %f, %f]\n\tPos. difference: [%f, %f, %f]\n\tRotation: [%f, %f, %f]\n\tRot. difference: [%f, %f, %f]\n\tHealth: %f\n"), GetId(), m_Position.x, m_Position.y, m_Position.z, m_RelPosition.x, m_RelPosition.y, m_RelPosition.z, m_Rotation.x, m_Rotation.y, m_Rotation.z, m_RelRotation.x, m_RelRotation.y, m_RelRotation.z);

	if (pStream->Write(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	return true;
}

bool CServerVehicle::WriteSyncPacket(Stream* pStream)
{
	if (!CServerEntity::WriteSyncPacket(pStream))
		return false;

	tVehicleSyncPacket Packet;

	Packet.rotationFront = m_RotationFront;
	Packet.rotationUp = m_RotationUp;
	Packet.rotationRight = m_RotationRight;
	Packet.health = m_Health;
	Packet.engineHealth = m_EngineHealth;
	Packet.fuel = m_Fuel;
	Packet.sound = m_SoundEnabled;
	Packet.engineOn = m_Engine;
	Packet.horn = m_Horn;
	Packet.siren = m_Siren;
	Packet.lights = m_Lights;
	Packet.gear = m_Gear;
	Packet.rpm = m_EngineRPM;
	Packet.accel = m_Accelerating;
	Packet.brake = m_Breakval;
	Packet.handBrake = m_Handbrake;
	Packet.speedLimit = m_SpeedLimit;
	Packet.clutch = m_Clutch;
	Packet.wheelAngle = m_WheelAngle;
	Packet.speed = m_Velocity;
	Packet.rotSpeed = m_RotVelocity;

	//_glogprintf(_gstr("Sent sync packet for vehicle #%d:\n\tPosition: [%f, %f, %f]\n\tPos. difference: [%f, %f, %f]\n\tRotation: [%f, %f, %f]\n\tRot. difference: [%f, %f, %f]\n\tHealth: %f\n"), GetId(), m_Position.x, m_Position.y, m_Position.z, m_RelPosition.x, m_RelPosition.y, m_RelPosition.z, m_Rotation.x, m_Rotation.y, m_Rotation.z, m_RelRotation.x, m_RelRotation.y, m_RelRotation.z);

	if (pStream->Write(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	return true;
}

void CServerVehicle::SetModel(const GChar* sModel)
{
	CServerEntity::SetModel(sModel);
}

void CServerVehicle::Remove(void)
{
	for (size_t i=0; i< m_pNetObjectMgr->m_Objects.GetSize(); i++)
	{
		if (m_pNetObjectMgr->m_Objects.IsUsedAt(i) && m_pNetObjectMgr->m_Objects.GetAt(i)->IsType(ELEMENT_PED))
		{
			CServerHuman* pServerHuman = static_cast<CServerHuman*>(m_pNetObjectMgr->m_Objects.GetAt(i));

			if (pServerHuman->IsInVehicle() && pServerHuman->m_nVehicleNetworkIndex == GetId())
			{
				if (pServerHuman->IsRandom())
					m_pNetObjectMgr->DestroyObject(pServerHuman, true, false);
				else
					pServerHuman->WarpIntoVehicle(nullptr, 0);
			}
		}
	}

	CServerEntity::Remove();
}

void CServerVehicle::Fix()
{
	m_Health = 1000.0f;

	Packet Packet(MAFIAPACKET_VEHICLE_FIX);
	Packet.Write<int32_t>(GetId());
	m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
}

void CServerVehicle::SetLocked(bool bLocked)
{
	if (m_Locked != bLocked)
	{
		m_Locked = bLocked;

		Packet Packet(MAFIAPACKET_VEHICLE_SETLOCKED);
		Packet.Write<int32_t>(GetId());
		Packet.Write<uint8_t>(bLocked);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

void CServerVehicle::SetEngine(bool bEngine)
{
	if (m_Engine != bEngine)
	{
		m_Engine = bEngine;

		Packet Packet(MAFIAPACKET_VEHICLE_SETENGINE);
		Packet.Write<int32_t>(GetId());
		Packet.Write<uint8_t>(bEngine);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

void CServerVehicle::SetSiren(bool bSiren)
{
	if (m_Siren != bSiren)
	{
		m_Siren = bSiren;

		Packet Packet(MAFIAPACKET_VEHICLE_SETSIREN);
		Packet.Write<int32_t>(GetId());
		Packet.Write<uint8_t>(bSiren);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

void CServerVehicle::SetLights(bool bLights)
{
	if (m_Lights != bLights)
	{
		m_Lights = bLights;

		Packet Packet(MAFIAPACKET_VEHICLE_SETLIGHTS);
		Packet.Write<int32_t>(GetId());
		Packet.Write<uint8_t>(bLights);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

void CServerVehicle::SetRoof(bool bRoof)
{
	if (m_Roof != bRoof)
	{
		m_Roof = bRoof;

		Packet Packet(MAFIAPACKET_VEHICLE_SETROOF);
		Packet.Write<int32_t>(GetId());
		Packet.Write<uint8_t>(bRoof);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

CServerHuman* CServerVehicle::GetOccupant(size_t Index)
{
	if (Index >= ARRAY_COUNT(m_pProbableOccupants))
		return nullptr;
	auto pProbableOccupant = m_pProbableOccupants[Index];
	if (pProbableOccupant != nullptr && pProbableOccupant->m_nVehicleNetworkIndex == GetId() && pProbableOccupant->m_ucSeat == Index)
		return pProbableOccupant;
	return nullptr;
}