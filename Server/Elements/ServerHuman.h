
#include "pch.h"
#include "Server.h"
#include "../MafiaServerManager.h"
#include "Elements.h"

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

	uint8_t m_AnimationStateLocal = 0;
	bool m_IsInAnimWithCarLocal = false;
	uint8_t m_AnimationState = 0;
	bool m_IsInAnimWithCar = false;
	float m_fInCarRotation = 0.0f;
	bool m_IsCrouching = false;
	bool m_IsAiming = false;
	bool m_IsShooting = false;
	int32_t m_iAnimStopTime = 0;
	int16_t m_WeaponId = 0;
	CVector3D m_Camera;

	float m_fCurrentRotation;
	float m_fHealth = 200;
	int32_t m_nVehicleNetworkIndex = INVALID_NETWORK_ID;
	size_t m_ucSeat = 0;
	uint32_t m_Behavior = 4;

	bool m_EnteringExitingVehicle = false;

	Galactic3D::Weak<CServerVehicle> m_pCurrentVehicle;

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