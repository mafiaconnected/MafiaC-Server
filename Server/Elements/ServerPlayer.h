#pragma once

class CServerPlayer : public CServerHuman
{
public:
	CServerPlayer(CMafiaServerManager* pServerManager);

	virtual ReflectedClass* GetReflectedClass() override;

	virtual bool ReadSyncPacket(Stream* pStream) override;
	virtual bool WriteSyncPacket(Stream* pStream) override;

	virtual bool SetPosition(const CVector3D& vecPos) override;
};