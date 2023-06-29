#pragma once

#include "ServerManager.h"
#include "Peer2PeerSystem.h"

class CMafiaServerManager;
class CMafiaServer;

enum eMafiaElementType
{
	ELEMENT_ENTITY = Galactic3D::ELEMENT_TRANSFORMABLE | 4,
	ELEMENT_PED = ELEMENT_ENTITY | 8,
	ELEMENT_PLAYER = ELEMENT_PED | 16,
	ELEMENT_VEHICLE = ELEMENT_ENTITY | 32,
	ELEMENT_DUMMY = ELEMENT_ENTITY | 64,
	ELEMENT_OBJECT = ELEMENT_ENTITY | 128,
};

class CMafiaClient : public CNetMachine
{
public:
	CMafiaClient(CNetObjectMgr* pServer);

	void SpawnPlayer(const CVector3D& vecPos, float fRotation, const GChar* szSkin);
};

class CMafiaServerManager : public CServerManager
{
public:
	CMafiaServerManager(Galactic3D::Context* pContext, CMafiaServer* pServer);

	CMafiaServer* m_pMafiaServer;

	CPeer2PeerSystem m_Peer2Peer;

	const char* m_szLevel;

	Galactic3D::EventHandlers::CEventType* m_pOnPedEnteredVehicleEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedEnteringVehicleEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedExitedVehicleEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedExitingVehicleEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedDeathEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedSpawnEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedFallEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedHitEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedShootEventType;
	Galactic3D::EventHandlers::CEventType* m_pOnPedThrowGrenadeEventType;

	Galactic3D::EventHandlers::CEventType* m_pOnReceivePacketEventType;

	Galactic3D::ReflectedClass* m_pServerEntityClass;
	Galactic3D::ReflectedClass* m_pServerHumanClass;
	Galactic3D::ReflectedClass* m_pServerPlayerClass;
	Galactic3D::ReflectedClass* m_pServerVehicleClass;
	Galactic3D::ReflectedClass* m_pServerDummyClass;
	Galactic3D::ReflectedClass* m_pServerObjectClass;

	virtual CNetObject* Create(int32_t nType) override;

	bool IsAnythingBlocking(CVector3D vecPos);

protected:
	void RegisterFunctions(CScripting* pScripting);
};