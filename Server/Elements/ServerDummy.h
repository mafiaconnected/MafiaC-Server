#pragma once

#include "ServerEntity.h"

class CServerDummy : public CServerEntity
{
public:
	CServerDummy(CMafiaServerManager* pServerManager);

	virtual ReflectedClass* GetReflectedClass() override;
};