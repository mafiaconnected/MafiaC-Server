
#include "pch.h"
#include "MafiaServer.h"
#include "MafiaServerManager.h"
#include "ServerVehicle.h"
#include "ServerHuman.h"
#include <Utils/VectorTools.h>

class CServerHuman;

CServerVehicle::CServerVehicle(CMafiaServerManager* pServerManager) : CServerEntity(pServerManager)
{
	m_Type = ELEMENT_VEHICLE;

	CMatrix3x3 mat3(CVector3D(0, 0, 0));
	m_RotationFront = mat3.GetXAxis();
	m_RotationUp = mat3.GetYAxis();
	m_RotationRight = mat3.GetZAxis();

	m_quatRot.x = 0.0f;
	m_quatRot.y = 0.0f;
	m_quatRot.z = 0.0f;
	m_quatRot.w = 0.0f;
}

CServerVehicle::~CServerVehicle()
{
	ClearDamageData();
}

ReflectedClass* CServerVehicle::GetReflectedClass()
{
	return static_cast<CMafiaServerManager*>(m_pNetObjectMgr)->m_pServerVehicleClass;
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
	m_Roof = Packet.roof;
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

	// Damage, only there when the vehicle has some (and never from a Mafia 2 client). It's the end of the packet, so a
	// bad one is just left off.
	ClearDamageData();
	uint32_t uiDamageSize = 0;
	if (pStream->Read(&uiDamageSize, sizeof(uiDamageSize)) == sizeof(uiDamageSize) && uiDamageSize != 0 && uiDamageSize <= VEHICLEDAMAGE_MAX_SIZE)
	{
		uint8_t* pDamage = (uint8_t*)GAlloc(uiDamageSize);
		if (pStream->Read(pDamage, uiDamageSize) == uiDamageSize)
			StoreDamageData(pDamage, uiDamageSize);
		GFree(pDamage);
	}

	//GChar szHost[256];
	//GetSyncer()->m_IPAddress.ToString(szHost, ARRAY_SIZE(szHost));
	//_glogprintf(_gstr("Got create packet for vehicle #%d (%s - ip %s):\n\tPosition: [%f, %f, %f]\n\tPos. difference: [%f, %f, %f]\n\tRotation: [%f, %f, %f]\n\tRot. difference: [%f, %f, %f]\n\tMafia Rotation Front: [%f, %f, %f]\n\tMafia Rotation Up: [%f, %f, %f]\n\tMafia Rotation Left: [%f, %f, %f]\n\tHealth: %f\n"), GetId(), GetSyncer()->GetName(), szHost, m_Position.x, m_Position.y, m_Position.z, m_RelPosition.x, m_RelPosition.y, m_RelPosition.z, m_Rotation.x, m_Rotation.y, m_Rotation.z, m_RelRotation.x, m_RelRotation.y, m_RelRotation.z, m_RotationFront.x, m_RotationFront.y, m_RotationFront.z, m_RotationUp.x, m_RotationUp.y, m_RotationUp.z, m_RotationRight.x, m_RotationRight.y, m_RotationRight.z, m_Health);

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
	m_quatRot = Packet.quatRot;
	m_Health = Packet.health;
	m_EngineHealth = Packet.engineHealth;
	m_Fuel = Packet.fuel;
	m_SoundEnabled = Packet.sound;
	m_Engine = Packet.engineOn;
	m_Horn = Packet.horn;
	m_Siren = Packet.siren;
	m_Lights = Packet.lights;
	m_Roof = Packet.roof;
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

	// Note: Commented out because GetSyncer() currently can be -1 here, which caused a server crash.
	//GChar szHost[256];
	//GetSyncer()->m_IPAddress.ToString(szHost, ARRAY_SIZE(szHost));
	//_glogprintf(_gstr("Got sync packet for vehicle #%d (%s - ip %s):\n\tPosition: [%f, %f, %f]\n\tPos. difference: [%f, %f, %f]\n\tRotation: [%f, %f, %f]\n\tRot. difference: [%f, %f, %f]\n\tMafia Rotation Front: [%f, %f, %f]\n\tMafia Rotation Up: [%f, %f, %f]\n\tMafia Rotation Left: [%f, %f, %f]\n\tHealth: %f\n"), GetId(), GetSyncer()->GetName(), szHost, m_Position.x, m_Position.y, m_Position.z, m_RelPosition.x, m_RelPosition.y, m_RelPosition.z, m_Rotation.x, m_Rotation.y, m_Rotation.z, m_RelRotation.x, m_RelRotation.y, m_RelRotation.z, m_RotationFront.x, m_RotationFront.y, m_RotationFront.z, m_RotationUp.x, m_RotationUp.y, m_RotationUp.z, m_RotationRight.x, m_RotationRight.y, m_RotationRight.z, m_Health);

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
	Packet.roof = m_Roof;
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

	//_glogprintf(_gstr("Sent create packet for vehicle #%d:\n\tPosition: [%f, %f, %f]\n\tPos. difference: [%f, %f, %f]\n\tRotation: [%f, %f, %f]\n\tRot. difference: [%f, %f, %f]\n\tMafia Rotation Front: [%f, %f, %f]\n\tMafia Rotation Up: [%f, %f, %f]\n\tMafia Rotation Left: [%f, %f, %f]\n\tHealth: %f\n"), GetId(), m_Position.x, m_Position.y, m_Position.z, m_RelPosition.x, m_RelPosition.y, m_RelPosition.z, m_Rotation.x, m_Rotation.y, m_Rotation.z, m_RelRotation.x, m_RelRotation.y, m_RelRotation.z, m_RotationFront.x, m_RotationFront.y, m_RotationFront.z, m_RotationUp.x, m_RotationUp.y, m_RotationUp.z, m_RotationRight.x, m_RotationRight.y, m_RotationRight.z, m_Health);

	if (pStream->Write(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	uint32_t uiDamageSize = (uint32_t)m_DamageDataSize;
	if (pStream->Write(&uiDamageSize, sizeof(uiDamageSize)) != sizeof(uiDamageSize))
		return false;

	if (m_DamageDataSize != 0 && pStream->Write(m_pDamageData, m_DamageDataSize) != m_DamageDataSize)
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
	Packet.quatRot = m_quatRot;
	Packet.health = m_Health;
	Packet.engineHealth = m_EngineHealth;
	Packet.fuel = m_Fuel;
	Packet.sound = m_SoundEnabled;
	Packet.engineOn = m_Engine;
	Packet.horn = m_Horn;
	Packet.siren = m_Siren;
	Packet.lights = m_Lights;
	Packet.roof = m_Roof;
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

	//_glogprintf(_gstr("Sent sync packet for vehicle #%d:\n\tPosition: [%f, %f, %f]\n\tPos. difference: [%f, %f, %f]\n\tRotation: [%f, %f, %f]\n\tRot. difference: [%f, %f, %f]\n\tMafia Rotation Front: [%f, %f, %f]\n\tMafia Rotation Up: [%f, %f, %f]\n\tMafia Rotation Left: [%f, %f, %f]\n\tHealth: %f\n"), GetId(), m_Position.x, m_Position.y, m_Position.z, m_RelPosition.x, m_RelPosition.y, m_RelPosition.z, m_Rotation.x, m_Rotation.y, m_Rotation.z, m_RelRotation.x, m_RelRotation.y, m_RelRotation.z, m_RotationFront.x, m_RotationFront.y, m_RotationFront.z, m_RotationUp.x, m_RotationUp.y, m_RotationUp.z, m_RotationRight.x, m_RotationRight.y, m_RotationRight.z, m_Health);

	if (pStream->Write(&Packet, sizeof(Packet)) != sizeof(Packet))
		return false;

	return true;
}

void CServerVehicle::SetModel(const GChar* sModel)
{
	CServerEntity::SetModel(sModel);
}

bool CServerVehicle::SetRotation(const CVector3D& vecRotation)
{
	CServerEntity::SetRotation(vecRotation);

	//CVector3D vec3 = CVecTools::ComputeDirEuler(CVecTools::RadToDeg(vecRotation.z));
	CMatrix3x3 mat3(vecRotation);
	m_RotationFront = mat3[0];
	m_RotationUp = mat3[1];
	m_RotationRight = mat3[2];

	return true;
}

void CServerVehicle::Remove()
{
	for (size_t i = 0; i < m_pNetObjectMgr->m_Objects.GetSize(); i++)
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

	// The syncer sends the repaired car's damage once it has repaired it
	ClearDamageData();

	Packet Packet(MAFIAPACKET_VEHICLE_FIX);
	Packet.Write<int32_t>(GetId());
	m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
}

void CServerVehicle::SetHealth(float fHealth)
{
	if (m_Health != fHealth)
	{
		m_Health = fHealth;

		Packet Packet(MAFIAPACKET_VEHICLE_SETHEALTH);
		Packet.Write<int32_t>(GetId());
		Packet.Write<float>(fHealth);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

bool CServerVehicle::StoreDamageData(const uint8_t* pData, size_t Size)
{
	if (Size < sizeof(tVehicleDamageHeader) || Size > VEHICLEDAMAGE_MAX_SIZE)
		return false;

	tVehicleDamageHeader Header;
	memcpy(&Header, pData, sizeof(Header));
	if (Header.magic != VEHICLEDAMAGE_MAGIC || Header.version != VEHICLEDAMAGE_VERSION)
		return false;

	// Only the clients can check the rest, they have the model
	uint64_t ExpectedSize = sizeof(Header) + (uint64_t)Header.lightCount * VEHICLEDAMAGE_LIGHT_SIZE + Header.partBytes + (uint64_t)Header.wheelCount * VEHICLEDAMAGE_WHEEL_SIZE;
	if (ExpectedSize != Size)
		return false;

	ClearDamageData();
	m_pDamageData = (uint8_t*)GAlloc(Size);
	_gmemcpy(m_pDamageData, pData, Size);
	m_DamageDataSize = Size;

	m_Health = Header.health;
	m_EngineHealth = Header.engineHealth;

	return true;
}

bool CServerVehicle::SetDamageData(const uint8_t* pData, size_t Size, CNetMachine* pExclude)
{
	if (!StoreDamageData(pData, Size))
		return false;

	if (!IsLocal())
	{
		Packet Packet(MAFIAPACKET_VEHICLE_SETDAMAGE);
		Packet.Write<int32_t>(GetId());
		Packet.Write<uint32_t>((uint32_t)Size);
		Packet.Write(pData, Size);

		for (size_t i = 0; i < MAX_MACHINES; i++)
		{
			auto pNetMachine = m_pNetObjectMgr->m_pNetMachines->GetMachine(i);
			if (pNetMachine != nullptr && pNetMachine != pExclude && pNetMachine->m_bJoined && !pNetMachine->m_bConsole && IsCreatedFor(pNetMachine))
				pNetMachine->SendPacket(&Packet);
		}
	}

	return true;
}

void CServerVehicle::ClearDamageData()
{
	if (m_pDamageData != nullptr)
	{
		GFree(m_pDamageData);
		m_pDamageData = nullptr;
	}
	m_DamageDataSize = 0;
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

void CServerVehicle::SetEngine(bool bEngine, bool bInstant)
{
	if (m_Engine != bEngine)
	{
		m_Engine = bEngine;

		Packet Packet(MAFIAPACKET_VEHICLE_SETENGINE);
		Packet.Write<int32_t>(GetId());
		Packet.Write<uint8_t>(bEngine);
		Packet.Write<uint8_t>(bInstant);
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

void CServerVehicle::SetFuel(float fFuel)
{
	if (m_Fuel != fFuel)
	{
		m_Fuel = fFuel;

		Packet Packet(MAFIAPACKET_VEHICLE_SETFUEL);
		Packet.Write<int32_t>(GetId());
		Packet.Write<float>(fFuel);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

void CServerVehicle::SetWheelAngle(float fWheelAngle)
{
	if (m_WheelAngle != fWheelAngle)
	{
		m_WheelAngle = fWheelAngle;

		Packet Packet(MAFIAPACKET_VEHICLE_SETWHEELANGLE);
		Packet.Write<int32_t>(GetId());
		Packet.Write<float>(fWheelAngle);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

void CServerVehicle::SetEngineRPM(float fEngineRPM)
{
	if (m_EngineRPM != fEngineRPM)
	{
		m_EngineRPM = fEngineRPM;

		Packet Packet(MAFIAPACKET_VEHICLE_SETENGINERPM);
		Packet.Write<int32_t>(GetId());
		Packet.Write<float>(fEngineRPM);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

void CServerVehicle::SetSpeedLimit(float fSpeedLimit)
{
	if (m_SpeedLimit != fSpeedLimit)
	{
		m_SpeedLimit = fSpeedLimit;

		Packet Packet(MAFIAPACKET_VEHICLE_SETSPEEDLIMIT);
		Packet.Write<int32_t>(GetId());
		Packet.Write<float>(fSpeedLimit);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

void CServerVehicle::SetEngineHealth(float fEngineHealth)
{
	if (m_EngineHealth != fEngineHealth)
	{
		m_EngineHealth = fEngineHealth;

		Packet Packet(MAFIAPACKET_VEHICLE_SETENGINEHEALTH);
		Packet.Write<int32_t>(GetId());
		Packet.Write<float>(fEngineHealth);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

void CServerVehicle::SetGear(uint32_t iGear)
{
	if (m_Gear != iGear)
	{
		m_Gear = iGear;

		Packet Packet(MAFIAPACKET_VEHICLE_SETGEAR);
		Packet.Write<int32_t>(GetId());
		Packet.Write<uint32_t>(iGear);
		m_pNetObjectMgr->SendObjectRelatedPacket(&Packet, this);
	}
}

CServerHuman* CServerVehicle::GetOccupant(int8_t Index)
{
	if (Index < 0 || Index >= ARRAY_COUNT(m_pProbableOccupants))
		return nullptr;
	auto pProbableOccupant = m_pProbableOccupants[Index];
	if (pProbableOccupant != nullptr && pProbableOccupant->m_nVehicleNetworkIndex == GetId() && pProbableOccupant->m_nSeat == Index)
		return pProbableOccupant;
	return nullptr;
}