
#include "pch.h"
#include "Server.h"
#include "../MafiaServerManager.h"
#include "Elements.h"

class CServerPlayer : public CServerHuman
{
public:
	CServerPlayer(CMafiaServerManager* pServerManager);

	virtual ReflectedClass* GetReflectedClass(void) override;

	virtual bool ReadSyncPacket(Stream* pStream) override;
	virtual bool WriteSyncPacket(Stream* pStream) override;

	virtual bool SetPosition(const CVector3D& vecPos) override;
};