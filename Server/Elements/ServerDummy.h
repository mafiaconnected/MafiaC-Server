#pragma once

#include "ServerEntity.h"

class CServerDummy : public CNetObject
{
public:
	CServerDummy(CMafiaServerManager* pServerManager);

	CVector3D m_Position = { 0, 0, 0 };

	virtual ReflectedClass* GetReflectedClass() override;

	virtual bool SetPosition(const CVector3D& vecPos) override;
	virtual bool GetPosition(CVector3D& vecPos) override;
};