#pragma once

#include "ServerManager.h"

class CServerEntity;
class CServerHuman;
class CServerPlayer;
class CServerVehicle;
class CMafiaServerManager;

enum eMafiaElementType
{
	ELEMENT_ENTITY = Galactic3D::ELEMENT_TRANSFORMABLE | 4,
	ELEMENT_PED = ELEMENT_ENTITY | 8,
	ELEMENT_PLAYER = ELEMENT_PED | 16,
	ELEMENT_VEHICLE = ELEMENT_ENTITY | 32,
};

class CMafiaClient : public CNetMachine
{
public:
	CMafiaClient(CNetObjectMgr* pServer);

	void SpawnPlayer(const CVector3D& vecPos, float fRotation, const GChar* szSkin);
};

class CWorld : public CBaseObject
{
public:
	CWorld(void);

	CMafiaServerManager* m_pManager;

	uint32_t m_LastTicks;
	uint64_t m_ulTimer;

	double m_fResyncTimer;

	void Process(double fDeltaTime);

	void SyncClient(CNetMachine* pClient);
};

class CMafiaServerManager : public CServerManager
{
public:
	CMafiaServerManager(Galactic3D::Context* pContext, CServer* pServer);

	const char* m_szLevel;

	Galactic3D::EventHandlers::CEventType* m_pOnPedEnteredVehicleEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedEnteringVehicleEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedExitedVehicleEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedExitingVehicleEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedDeathEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedSpawnEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedFallEventType;

	Galactic3D::ReflectedClass* m_pServerEntityClass;
	Galactic3D::ReflectedClass* m_pServerHumanClass;
	Galactic3D::ReflectedClass* m_pServerPlayerClass;
	Galactic3D::ReflectedClass* m_pServerVehicleClass;

	Galactic3D::Weak<CServerVehicle> m_rgpVehicles[128];
	Galactic3D::Weak<CServerHuman> m_rgpPeds[128];
	Galactic3D::Weak<CServerPlayer> m_rgpPlayers[128];

	virtual CNetObject* Create(int32_t nType) override;

	bool IsAnythingBlocking(CVector3D vecPos);

protected:
	void RegisterFunctions(CScripting* pScripting);
};

class CServerEntity : public CNetObject
{
public:
	CServerEntity(CMafiaServerManager* pServerManager);

	CVector3D m_Position;
	CVector3D m_RelPosition;
	CVector3D m_Rotation;
	CVector3D m_RelRotation;
	GChar m_szModel[64];
	unsigned char m_ucInterior;
	int32_t m_nRelativeElement;
	CVector3D m_vecRelativePos;
	int32_t m_nRef;
	float m_fStreamInDistance;
	float m_fStreamOutDistance;
	CMafiaServerManager* m_pServerManager;

	virtual void SetHeading(float fHeading);
	virtual float GetHeading(void);

	inline void SetStreamInDistance(float fStreamInDistance) { m_fStreamInDistance = fStreamInDistance; }
	inline float GetStreamInDistance() { return m_fStreamInDistance; }

	inline void SetStreamOutDistance(float fStreamOutDistance) { m_fStreamOutDistance = fStreamOutDistance; }
	inline float GetStreamOutDistance() { return m_fStreamOutDistance; }

	virtual ReflectedClass* GetReflectedClass(void) override;

	virtual bool SetPosition(const CVector3D& vecPos) override;
	virtual bool GetPosition(CVector3D& vecPos) override;

	virtual bool SetRotation(const CVector3D& vecRotation) override;
	virtual bool GetRotation(CVector3D& vecRotation) override;

	virtual bool ShouldExistForMachine(CNetMachine* pClient) override;
	virtual bool ShouldDeleteForMachine(CNetMachine* pClient) override;

	virtual bool ReadCreatePacket(Stream* pStream) override;
	virtual bool ReadSyncPacket(Stream* pStream) override;

	virtual bool WriteCreatePacket(Stream* pStream) override;
	virtual bool WriteSyncPacket(Stream* pStream) override;

	bool IsRandom();
	virtual void SetModel(const GChar* modelName);
};

class InventoryItem
{
public:
	unsigned short weapId = 0;
	unsigned short ammo1 = 0;
	unsigned short ammo2 = 0;
};

class CServerHuman : public CServerEntity
{
public:
	CServerHuman(CMafiaServerManager* pServerManager);

	InventoryItem items[8];

	uint8_t m_AnimationState = 0;
	bool m_IsCrouching = false;
	bool m_IsAiming = false;

	float m_fCurrentRotation;
	float m_fHealth = 200;
	int32_t m_nVehicleNetworkIndex = INVALID_NETWORK_ID;
	size_t m_ucSeat = 0;
	uint32_t m_Behavior = 4;

	Galactic3D::Weak<CServerVehicle> m_pCurrentVehicle;

	virtual void SetHeading(float fHeading) override;
	virtual float GetHeading(void) override;

	virtual void SetModel(const GChar* sModel) override;

	virtual ReflectedClass* GetReflectedClass(void) override;

	virtual void OnCreated(void) override;
	void OnSpawned();

	virtual void Shoot(bool state, const CVector3D& distPos);
	void Jump();
	void ThrowGrenade(const CVector3D& dstPos);
	void Reload();
	void HolsterWeapon();

	virtual void Kill();

	virtual bool IsInVehicle(void);
	virtual bool CanEnterVehicle(CServerVehicle* pVehicle, unsigned char ucSeat);
	virtual bool CanExitVehicle(void);

	virtual void WarpIntoVehicle(CServerVehicle* pVehicle, unsigned char ucSeat);

	virtual bool ShouldExistForMachine(CNetMachine* pClient) override;
	virtual bool ShouldDeleteForMachine(CNetMachine* pClient) override;

	virtual bool ReadCreatePacket(Stream* pStream) override;
	virtual bool ReadSyncPacket(Stream* pStream) override;

	virtual bool WriteCreatePacket(Stream* pStream) override;
	virtual bool WriteSyncPacket(Stream* pStream) override;

	virtual InventoryItem GetInventoryItem(int index) { if (index >= 0 && index < 8) return items[index]; else return { 0 }; }
	virtual InventoryItem* GetInventory() { return items; }
	virtual void ClearInventory();
	void GiveWeapon(unsigned short ucWeapon, unsigned short ucAmmo1, unsigned short ucAmmo2);
	bool HasWeapon(unsigned short ucWeapon);
	int GetFirstEmptyWeaponIndex();
	int GetIndexOfWeapon(unsigned short ucWeapon);
	void TakeWeapon(unsigned short ucWeapon);
	void DropWeapon();

	void SetHealth(float fHealth);
};

class CServerPlayer : public CServerHuman
{
public:
	CServerPlayer(CMafiaServerManager* pServerManager);

	virtual ReflectedClass* GetReflectedClass(void) override;

	virtual bool ReadSyncPacket(Stream* pStream) override;
	virtual bool WriteSyncPacket(Stream* pStream) override;
};

class CServerVehicle : public CServerEntity
{
public:
	CServerVehicle(CMafiaServerManager* pServerManager);

	float m_Health;
	float m_EngineHealth;
	float m_Fuel;

	bool m_SoundEnabled;
	bool m_Engine;
	bool m_Horn;
	bool m_Siren;
	bool m_Locked;
	bool m_Roof;
	bool m_Lights;

	int m_Gear;

	float m_EngineRPM;
	float m_Accelerating;
	float m_Breakval;
	float m_Handbrake;
	float m_SpeedLimit;
	float m_Clutch;

	float m_WheelAngle;

	CVector3D m_Velocity;
	CVector3D m_RotVelocity;

	uint32_t m_Damage1;
	uint32_t m_Damage2;

	// Occupants probably in this vehicle
	Weak<CServerHuman> m_pProbableOccupants[4];

	virtual ReflectedClass* GetReflectedClass(void) override;

	virtual bool ShouldExistForMachine(CNetMachine* pClient) override;
	virtual bool ShouldDeleteForMachine(CNetMachine* pClient) override;

	virtual bool ReadCreatePacket(Stream* pStream) override;
	virtual bool ReadSyncPacket(Stream* pStream) override;

	virtual bool WriteCreatePacket(Stream* pStream) override;
	virtual bool WriteSyncPacket(Stream* pStream) override;

	virtual void SetModel(const GChar* sModelName);

	virtual void Remove(void) override;

	void Fix();

	bool GetLocked(void) { return m_Locked; }
	void SetLocked(bool bLocked);

	bool GetEngine(void) { return m_Engine; }
	void SetEngine(bool bEngine);

	bool GetSiren(void) { return m_Siren; }
	void SetSiren(bool bSiren);

	bool GetLights(void) { return m_Lights; }
	void SetLights(bool bLights);

	bool GetRoof(void) { return m_Roof; }
	void SetRoof(bool bRoof);

	CServerHuman* GetOccupant(size_t Index);
};
