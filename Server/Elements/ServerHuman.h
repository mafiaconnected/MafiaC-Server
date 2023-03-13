#pragma once

#include "ServerEntity.h"

class CServerVehicle;

class InventoryItem
{
public:
	uint16_t weapId;
	uint16_t ammo1;
	uint16_t ammo2;
};

class CServerHuman : public CServerEntity
{
public:
	CServerHuman(CMafiaServerManager* pServerManager);

	InventoryItem items[8];

	uint8_t m_AnimationStateLocal;
	bool m_IsInAnimWithCarLocal;
	uint8_t m_AnimationState;
	bool m_IsInAnimWithCar;
	float m_fInCarRotation;
	bool m_IsCrouching;
	bool m_IsAiming;
	bool m_IsShooting;
	int32_t m_iAnimStopTime;
	int16_t m_WeaponId;
	CVector3D m_Camera;

	float m_fCurrentRotation;
	float m_fHealth;
	int32_t m_nVehicleNetworkIndex;
	int8_t m_nSeat;
	uint32_t m_Behavior;

	bool m_EnteringExitingVehicle;

	Galactic3D::Weak<CServerVehicle> m_pCurrentVehicle;

	virtual void SetModel(const GChar* sModel) override;

	virtual ReflectedClass* GetReflectedClass() override;

	virtual void OnCreated() override;
	void OnSpawned();

	virtual void Shoot(bool state, const CVector3D& distPos);
	void Jump();
	void ThrowGrenade(const CVector3D& dstPos);
	void Reload();
	void HolsterWeapon();

	virtual void Kill();

	virtual bool IsInVehicle();
	virtual bool CanEnterVehicle(CServerVehicle* pVehicle, int8_t iSeat);
	virtual bool CanExitVehicle();

	virtual void WarpIntoVehicle(CServerVehicle* pVehicle, int8_t iSeat);

	virtual bool CanExistForMachine(CNetMachine* pClient) override;

	virtual bool ReadCreatePacket(Stream* pStream) override;
	virtual bool ReadSyncPacket(Stream* pStream) override;

	virtual bool WriteCreatePacket(Stream* pStream) override;
	virtual bool WriteSyncPacket(Stream* pStream) override;

	virtual InventoryItem GetInventoryItem(int index) { if (index >= 0 && index < 8) return items[index]; else return { 0 }; }
	virtual InventoryItem* GetInventory() { return items; }
	virtual void ClearInventory();
	void GiveWeapon(uint16_t ucWeapon, uint16_t ucAmmo1, uint16_t ucAmmo2);
	bool HasWeapon(uint16_t ucWeapon);
	int GetFirstEmptyWeaponIndex();
	int GetIndexOfWeapon(uint16_t ucWeapon);
	void TakeWeapon(uint16_t ucWeapon);
	void DropWeapon();

	void SetHealth(float fHealth);
};