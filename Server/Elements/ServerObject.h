#pragma once

#include "ServerEntity.h"

class CServerObject : public CServerEntity
{
public:
	CServerObject(CMafiaServerManager* pServerManager);

	virtual ReflectedClass* GetReflectedClass() override;
};