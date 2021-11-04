
#include "pch.h"
#include "Server.h"
#include "../MafiaServerManager.h"
#include "Elements.h"

class CServerVehicle : public CServerEntity
{
public:
	CServerVehicle(CMafiaServerManager* pServerManager);

	CVector3D m_RotationFront;
	CVector3D m_RotationUp;
	CVector3D m_RotationRight;
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
	virtual bool SetRotation(const CVector3D& vecRotation) override;

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

	float GetFuel(void) { return m_Fuel; }
	void SetFuel(float fFuel);

	float GetEngineRPM(void) { return m_EngineRPM; }
	void SetEngineRPM(float fEngineRPM);

	float GetWheelAngle(void) { return m_WheelAngle; }
	void SetWheelAngle(float fWheelAngle);

	float GetSpeedLimit(void) { return m_SpeedLimit; }
	void SetSpeedLimit(float fSpeedLimit);

	float GetEngineHealth(void) { return m_EngineHealth; }
	void SetEngineHealth(float fEngineHealth);

	//float GetSpeed(void) { return m_Speed; }
	//void SetSpeed(float fSpeed);

	//void GetVelocity(CVector3D& vecVelocity);
	//void SetVelocity(const CVector3D& vecVelocity);

	//void GetRotationVelocity(CVector3D& vecRotationVelocity);
	//void SetRotationVelocity(const CVector3D& vecRotationVelocity);

	CServerHuman* GetOccupant(size_t Index);
};