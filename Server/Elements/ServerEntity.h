#pragma once

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

	virtual void SetHeading(float fHeading);
	virtual float GetHeading();

	virtual ReflectedClass* GetReflectedClass() override;

	virtual bool SetPosition(const CVector3D& vecPos) override;
	virtual bool GetPosition(CVector3D& vecPos) override;

	virtual bool SetRotation(const CVector3D& vecRotation) override;
	virtual bool GetRotation(CVector3D& vecRotation) override;

	virtual bool SetVelocity(const CVector3D& vecVelocity);
	virtual bool GetVelocity(CVector3D& vecVelocity);

	virtual bool SetRotationVelocity(const CVector3D& vecRotationVelocity);
	virtual bool GetRotationVelocity(CVector3D& vecRotationVelocity);

	virtual bool ReadCreatePacket(Stream* pStream) override;
	virtual bool ReadSyncPacket(Stream* pStream) override;

	virtual bool WriteCreatePacket(Stream* pStream) override;
	virtual bool WriteSyncPacket(Stream* pStream) override;

	bool IsRandom();
	virtual void SetModel(const GChar* modelName);
};