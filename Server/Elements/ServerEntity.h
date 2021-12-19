
#include "pch.h"
#include "Server.h"
#include "../MafiaServerManager.h"
#include "Elements.h"

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

	virtual bool SetVelocity(const CVector3D& vecVelocity);
	virtual bool GetVelocity(CVector3D& vecVelocity);

	virtual bool SetRotationVelocity(const CVector3D& vecRotationVelocity);
	virtual bool GetRotationVelocity(CVector3D& vecRotationVelocity);

	virtual bool ShouldExistForMachine(CNetMachine* pClient) override;
	virtual bool ShouldDeleteForMachine(CNetMachine* pClient) override;

	virtual bool ReadCreatePacket(Stream* pStream) override;
	virtual bool ReadSyncPacket(Stream* pStream) override;

	virtual bool WriteCreatePacket(Stream* pStream) override;
	virtual bool WriteSyncPacket(Stream* pStream) override;

	bool IsRandom();
	virtual void SetModel(const GChar* modelName);
};