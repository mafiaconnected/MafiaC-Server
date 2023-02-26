#include "Server.h"
#include "../MafiaServerManager.h"
#include "Elements.h"

class InventoryItem
{
public:
	unsigned short weapId;
	unsigned short ammo1;
	unsigned short ammo2;
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
	virtual bool CanEnterVehicle(CServerVehicle* pVehicle, int8_t iSeat);
	virtual bool CanExitVehicle(void);

	virtual void WarpIntoVehicle(CServerVehicle* pVehicle, int8_t iSeat);

	virtual bool CanExistForMachine(CNetMachine* pClient) override;

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