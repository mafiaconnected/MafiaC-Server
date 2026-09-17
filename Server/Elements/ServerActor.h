#pragma once

#include "ServerEntity.h"

class CServerActor : public CServerEntity
{
public:
	CServerActor(CMafiaServerManager* pServerManager);

	virtual ReflectedClass* GetReflectedClass() override;
};
