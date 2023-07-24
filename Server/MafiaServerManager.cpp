

#include "pch.h"
#include "MafiaServer.h"
#include "MafiaServerManager.h"
#include "Peer2PeerSystem.h"
#include "Elements/Elements.h"

#include "Utils/VectorTools.h"

class CServerVehicle;
class CServerHuman;
class CServerPlayer;
class CServerDummy;
class CServerObject;

CMafiaClient::CMafiaClient(CNetObjectMgr* pServer) :
	CNetMachine(pServer)
{
}

void CMafiaClient::SpawnPlayer(const CVector3D& vecPos, float fRotation, const GChar* modelName)
{
	//DespawnPlayer();

	auto pPlayer = Strong<CServerPlayer>::New(static_cast<CServerPlayer*>(m_pNetObjectMgr->Create(ELEMENT_PLAYER)));

	//_glogprintf(_gstr("SpawnPlayer - Player model: %s"), modelName);

	_gstrcpy_s(pPlayer->m_szModel, ARRAY_COUNT(pPlayer->m_szModel), modelName);
	pPlayer->SetSyncer(this, true);
	pPlayer->SetHeading(fRotation);

	// The special "set position" packet was fucking this up
	// I removed the packet send for now. This will just update the net object's position data
	pPlayer->SetPosition(vecPos);
	pPlayer->SetRotation(CVecTools::ComputeDirEuler(fRotation));

	if (!m_pNetObjectMgr->RegisterNetObject(pPlayer))
		return;

	//m_pNetObjectMgr->SendCreatePacket(this, pPlayer, false); // this call is useless

	SetPlayer(pPlayer);
	pPlayer->OnSpawned();
}

CMafiaServerManager::CMafiaServerManager(Context* pContext, CMafiaServer* pServer) :
	CServerManager(pContext, pServer),
	m_pMafiaServer(pServer),
	m_Peer2Peer(this)
{
	m_Games.Register(_gstr("mafia:one"), GAME_MAFIA_ONE);
	m_Games.Register(_gstr("mafia:two"), GAME_MAFIA_TWO);
	m_Games.Register(_gstr("mafia:three"), GAME_MAFIA_THREE);
	m_Games.Register(_gstr("mafia:one_de"), GAME_MAFIA_ONE_DE);

	m_pOnPedEnteredVehicleEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedEnteredVehicle"), _gstr("Called when a ped is finished entering a vehicle."), 3, true);
	m_pOnPedExitedVehicleEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedExitedVehicle"), _gstr("Called when a ped has finished exiting a vehicle."), 3, true);
	m_pOnPedEnteringVehicleEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedEnteringVehicle"), _gstr("Called when a ped is started entering a vehicle."), 3, true);
	m_pOnPedExitingVehicleEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedExitingVehicle"), _gstr("Called when a ped has started exiting a vehicle."), 3, true);
	m_pOnPedDeathEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedDeath"), _gstr("Called when a ped dies."), 2, true);
	m_pOnPedSpawnEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedSpawn"), _gstr("Called when a ped is spawned."), 1, true);
	m_pOnPedFallEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedFall"), _gstr("Called when a ped falls."), 1, true);
	m_pOnPedHitEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedHit"), _gstr("Called when a ped is hit."), 7, true);
	m_pOnPedShootEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedShoot"), _gstr("Called when a ped shoots."), 3, true);
	m_pOnPedThrowGrenadeEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedThrowGrenade"), _gstr("Called when a ped throws a grenade."), 2, true);

	//m_pOnReceivePacketEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnReceivePacket"), _gstr("Called when a packet is received"), 2, true);

	auto pMafia = m_pServer->m_ResourceMgr.m_pScripting->m_Global.AddNamespace(_gstr("mafia"));
	pMafia->SetAlias(_gstr("game"));

	m_pServerEntityClass = pMafia->NewClass(_gstr("Entity"), m_pNetObjectClass);
	m_pServerHumanClass = pMafia->NewClass(_gstr("Ped"), m_pServerEntityClass);
	m_pServerPlayerClass = pMafia->NewClass(_gstr("Player"), m_pServerHumanClass);
	m_pServerVehicleClass = pMafia->NewClass(_gstr("Vehicle"), m_pServerEntityClass);
	m_pServerDummyClass = pMafia->NewClass(_gstr("Dummy"), m_pServerEntityClass);
	m_pServerObjectClass = pMafia->NewClass(_gstr("Object"), m_pServerEntityClass);

	auto pDefineHandlers = m_pServer->m_ResourceMgr.m_pDefineHandlers;

	pDefineHandlers->Define(_gstr("SERVER_VERSION_MAJOR"), MAFIAC_SERVER_MAJOR);
	pDefineHandlers->Define(_gstr("SERVER_VERSION_MINOR"), MAFIAC_SERVER_MINOR);
	pDefineHandlers->Define(_gstr("SERVER_VERSION_PATCH"), MAFIAC_SERVER_PATCH);

	pDefineHandlers->Define(_gstr("GAME_UNKNOWN"), GAME_UNKNOWN);
	pDefineHandlers->Define(_gstr("GAME_MAFIA_ONE"), GAME_MAFIA_ONE);
	pDefineHandlers->Define(_gstr("GAME_MAFIA_TWO"), GAME_MAFIA_TWO);
	pDefineHandlers->Define(_gstr("GAME_MAFIA_THREE"), GAME_MAFIA_THREE);
	pDefineHandlers->Define(_gstr("GAME_MAFIA_ONE_DE"), GAME_MAFIA_ONE_DE);

	// Compatibility with GTA Connected
	pDefineHandlers->Define(_gstr("GAME_GTA_III"), 1);
	pDefineHandlers->Define(_gstr("GAME_GTA_VC"), 2);
	pDefineHandlers->Define(_gstr("GAME_GTA_SA"), 3);
	pDefineHandlers->Define(_gstr("GAME_GTA_IV"), 5);
	pDefineHandlers->Define(_gstr("GAME_GTA_IV_EFLC"), 6);

	pDefineHandlers->Define(_gstr("ELEMENT_ELEMENT"), ELEMENT_ELEMENT);
	pDefineHandlers->Define(_gstr("ELEMENT_ENTITY"), ELEMENT_ENTITY);
	pDefineHandlers->Define(_gstr("ELEMENT_PED"), ELEMENT_PED);
	pDefineHandlers->Define(_gstr("ELEMENT_PLAYER"), ELEMENT_PLAYER);
	pDefineHandlers->Define(_gstr("ELEMENT_VEHICLE"), ELEMENT_VEHICLE);
	pDefineHandlers->Define(_gstr("ELEMENT_DUMMY"), ELEMENT_DUMMY);
	pDefineHandlers->Define(_gstr("ELEMENT_OBJECT"), ELEMENT_OBJECT);

	RegisterFunctions(m_pServer->m_ResourceMgr.m_pScripting);
}

/*
CNetObject* CMafiaServerManager::Create(int32_t nType)
{
	CServerHuman* human;
	CServerVehicle* vehicle;
	CServerPlayer* player;

	switch (nType)
	{A
		case ELEMENT_VEHICLE:
			vehicle = new CServerVehicle(this);
			for (int i = 0; i < 128; i++)
			{
				if (m_rgpVehicles[i].IsNull())
				{
					m_rgpVehicles[i] = vehicle;
					return vehicle;
				}
			}

			break;

		case ELEMENT_PLAYER:
			player = new CServerPlayer(this);

			for (int i = 0; i < 128; i++)
			{
				if (m_rgpPlayers[i].IsNull())
				{
					m_rgpPlayers[i] = player;
					return player;
				}
			}
		case ELEMENT_PED:
			human = new CServerHuman(this);

			for (int i = 0; i < 128; i++)
			{
				if (m_rgpPeds[i].IsNull())
				{
					m_rgpPeds[i] = human;
					return human;
				}
			}
		default:
			break;
	}
	return nullptr;
}
*/

CNetObject* CMafiaServerManager::Create(int32_t nType)
{
	switch (nType)
	{
	case ELEMENT_ELEMENT:
		return new CServerEntity(this);
	case ELEMENT_VEHICLE:
		return new CServerVehicle(this);
	case ELEMENT_PLAYER:
		return new CServerPlayer(this);
	case ELEMENT_PED:
		return new CServerHuman(this);
	case ELEMENT_DUMMY:
		return new CServerDummy(this);
	case ELEMENT_OBJECT:
		return new CServerObject(this);
	default:
		break;
	}
	return nullptr;
}

bool CMafiaServerManager::IsAnythingBlocking(CVector3D vecPos)
{
	for (size_t i = 0; i < m_Objects.GetSize(); i++)
	{
		if (m_Objects.IsUsedAt(i) && m_Objects.GetAt(i)->IsType(ELEMENT_ENTITY))
		{
			auto pEntity = static_cast<CServerEntity*>(m_Objects.GetAt(i));

			CVector3D vecEntityPos;
			if (pEntity->GetPosition(vecEntityPos))
			{
				float fDistance = (vecEntityPos - vecPos).GetLength();
				if (fDistance < 10.0f)
					return true;
			}
		}
	}

	return false;
}

static bool FunctionEntityGetVelocity(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass, &pServerEntity))
		return false;

	CVector3D vecVelocity;
	pServerEntity->GetRotationVelocity(vecVelocity);

	pState->ReturnVector3D(vecVelocity);
	return true;
}

static bool FunctionEntitySetVelocity(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass, &pServerEntity))
		return false;

	CVector3D vecVelocity;
	if (!pState->CheckVector3D(0, vecVelocity))
		return false;

	pServerEntity->SetVelocity(vecVelocity);
	return true;
}

static bool FunctionEntityGetRotationVelocity(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass, &pServerEntity))
		return false;

	CVector3D vecRotationVelocity;
	pServerEntity->GetRotationVelocity(vecRotationVelocity);

	pState->ReturnVector3D(vecRotationVelocity);
	return true;
}

static bool FunctionEntitySetRotationVelocity(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass, &pServerEntity))
		return false;

	CVector3D vecRotationVelocity;
	if (!pState->CheckVector3D(0, vecRotationVelocity))
		return false;

	pServerEntity->SetRotationVelocity(vecRotationVelocity);
	return true;
}

static bool FunctionVehicleGetGear(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;

	pState->ReturnNumber(pServerVehicle->m_Gear);
	return true;
}

static bool FunctionVehicleSetGear(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;

	uint32_t iGear;
	if (!pState->CheckNumber(0, iGear))
		return false;

	pServerVehicle->SetGear(iGear);
	return true;
}

static bool FunctionEntityGetModel(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass,&pServerEntity))
		return false;

	CString model(false, pServerEntity->m_szModel);

	pState->ReturnString(model.CString());
	return true;
}

static bool FunctionEntitySetModel(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass,&pServerEntity))
		return false;
	const GChar* sModel = pState->CheckString(0);
	if (!sModel)
		return false;

	pServerEntity->SetModel(sModel);
	return true;
}

static bool FunctionEntityGetHeading(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass, &pServerEntity))
		return false;
	pState->ReturnNumber(pServerEntity->GetHeading());
	return true;
}

static bool FunctionEntitySetHeading(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass, &pServerEntity))
		return false;
	float fHeading;
	if (!pState->CheckNumber(0, fHeading))
		return false;
	pServerEntity->SetHeading(fHeading);
	return true;
}

static bool FunctionVehicleGetLocked(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	pState->ReturnBoolean(pServerVehicle->GetLocked());
	return true;
}

static bool FunctionVehicleSetLocked(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	bool bLocked;
	if (!pState->CheckBoolean(0, bLocked))
		return false;
	pServerVehicle->SetLocked(bLocked);
	return true;
}

static bool FunctionVehicleGetEngine(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	pState->ReturnBoolean(pServerVehicle->GetEngine());
	return true;
}

static bool FunctionVehicleSetEngine(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	bool bEngine;
	if (!pState->CheckBoolean(0, bEngine))
		return false;
	pServerVehicle->SetEngine(bEngine, true);
	return true;
}


static bool FunctionVehicleSetEngineDetailed(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	bool bEngine;
	if (!pState->CheckBoolean(0, bEngine))
		return false;
	bool bInstant = 1;
	if (!pState->CheckBoolean(1, bInstant))
		return false;
	pServerVehicle->SetEngine(bEngine, bInstant);
	return true;
}

static bool FunctionVehicleGetLights(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	pState->ReturnBoolean(pServerVehicle->GetLights());
	return true;
}

static bool FunctionVehicleSetLights(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	bool bLights;
	if (!pState->CheckBoolean(0, bLights))
		return false;
	pServerVehicle->SetLights(bLights);
	return true;
}

static bool FunctionVehicleGetRoof(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	pState->ReturnBoolean(pServerVehicle->GetRoof());
	return true;
}

static bool FunctionVehicleSetRoof(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	bool bRoof;
	if (!pState->CheckBoolean(0, bRoof))
		return false;
	pServerVehicle->SetRoof(bRoof);
	return true;
}

static bool FunctionVehicleGetSiren(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	pState->ReturnBoolean(pServerVehicle->GetSiren());
	return true;
}

static bool FunctionVehicleSetSiren(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	bool bSiren;
	if (!pState->CheckBoolean(0, bSiren))
		return false;
	pServerVehicle->SetSiren(bSiren);
	return true;
}

static bool FunctionVehicleGetFuel(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	pState->ReturnNumber(pServerVehicle->GetFuel());
	return true;
}

static bool FunctionVehicleSetFuel(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	float fFuel;
	if (!pState->CheckNumber(0, fFuel))
		return false;
	pServerVehicle->SetFuel(fFuel);
	return true;
}

static bool FunctionVehicleGetWheelAngle(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	pState->ReturnNumber(pServerVehicle->GetWheelAngle());
	return true;
}

static bool FunctionVehicleSetWheelAngle(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	float fWheelAngle;
	if (!pState->CheckNumber(0, fWheelAngle))
		return false;
	pServerVehicle->SetWheelAngle(fWheelAngle);
	return true;
}

static bool FunctionVehicleGetEngineRPM(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	pState->ReturnNumber(pServerVehicle->GetEngineRPM());
	return true;
}

static bool FunctionVehicleSetEngineRPM(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	float fEngineRPM;
	if (!pState->CheckNumber(0, fEngineRPM))
		return false;
	pServerVehicle->SetEngineRPM(fEngineRPM);
	return true;
}

static bool FunctionVehicleGetEngineHealth(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	pState->ReturnNumber(pServerVehicle->GetEngineHealth());
	return true;
}

static bool FunctionVehicleSetEngineHealth(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	float fEngineHealth;
	if (!pState->CheckNumber(0, fEngineHealth))
		return false;
	pServerVehicle->SetEngineHealth(fEngineHealth);
	return true;
}

/*
static bool FunctionVehicleGetSpeed(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	pState->ReturnNumber(pServerVehicle->GetSpeed());
	return true;
}

static bool FunctionVehicleSetSpeed(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	float fSpeed;
	if (!pState->CheckNumber(0, fSpeed))
		return false;
	pServerVehicle->SetSpeed(fSpeed);
	return true;
}
*/

static bool FunctionVehicleGetSpeedLimit(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	pState->ReturnNumber(pServerVehicle->GetSpeedLimit());
	return true;
}

static bool FunctionVehicleSetSpeedLimit(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	float fSpeedLimit;
	if (!pState->CheckNumber(0, fSpeedLimit))
		return false;
	pServerVehicle->SetSpeedLimit(fSpeedLimit);
	return true;
}

static bool FunctionVehicleFix(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	pServerVehicle->Fix();
	return true;
}

static bool FunctionVehicleGetOccupant(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;
	int iSeatId;
	if (!pState->CheckNumber(0, iSeatId))
		return false;
	pState->ReturnObject((CBaseObject*)pServerVehicle->GetOccupant(iSeatId));
	return true;
}

static bool FunctionVehicleGetOccupants(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerVehicle* pServerVehicle;
	if (!pState->GetThis(pServerManager->m_pServerVehicleClass, &pServerVehicle))
		return false;

	auto pArray = new CArgumentArray();
	for (size_t i = 0; i < ARRAY_COUNT(pServerVehicle->m_pProbableOccupants); i++)
	{
		CServerHuman* pServerPed = pServerVehicle->GetOccupant(i);
		if (pServerPed != nullptr)
		{
			pArray->AddObject((CBaseObject*)pServerPed);
		}
	}

	pState->ReturnAndOwn(pArray);
	return true;
}

static bool FunctionPedGetSkin(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerHuman* pServerHuman;
	if (!pState->GetThis(pServerManager->m_pServerHumanClass,&pServerHuman))
		return false;

	CString model(false, pServerHuman->m_szModel);

	pState->ReturnString(model.CString());
	return true;
}

static bool FunctionPedSetSkin(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerHuman* pServerHuman;
	if (!pState->GetThis(pServerManager->m_pServerHumanClass,&pServerHuman))
		return false;
	const GChar* sModel = pState->CheckString(0);
	if (!sModel)
		return false;

	pServerHuman->SetModel(sModel);
	return true;
}

static bool FunctionHumanGiveWeapon(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerHuman* pServerHuman;
	if (!pState->GetThis(pServerManager->m_pServerHumanClass, &pServerHuman))
		return false;

	uint16_t weaponId = 0;
	if (!pState->CheckNumber(0, weaponId))
		return false;

	uint16_t ammo1 = 999;
	if (!pState->CheckNumber(1, ammo1))
		return false;

	uint16_t ammo2 = 999;
	if (!pState->CheckNumber(2, ammo2))
		return false;

	pServerHuman->GiveWeapon(weaponId, ammo1, ammo2);
	return true;
}

static bool FunctionGameSetLevel(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;

	const GChar* level = pState->CheckString(0);

	pServerManager->m_pMafiaServer->SetMapName(level);

	return true;
}

static bool FunctionGameGetLevel(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;

	pState->ReturnString(pServerManager->m_pMafiaServer->m_szMap);

	return true;
}

static bool FunctionSpawnPlayer(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CMafiaClient* pClient;
	if (!pState->CheckClass(pServerManager->m_pServer->m_pManager->m_pNetMachineClass, 0, false, &pClient))
		return false;

	CVector3D vecPos;
	if (!pState->CheckVector3D(1, vecPos))
		return false;

	float fRotation = 0.0f;
	if (argc > 2 && !pState->CheckNumber(2, fRotation))
		return false;

	const GChar* sModel = pState->CheckString(3);

	pClient->SpawnPlayer(vecPos, fRotation, sModel);
	return true;
}

static bool FunctionPlayerHudMsg(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CMafiaClient* pClient;
	if (!pState->CheckClass(pServerManager->m_pServer->m_pManager->m_pNetMachineClass, 0, false, &pClient))
		return false;

	const GChar* msg = pState->CheckString(1);
	if (!msg) return false;

	Packet Packet(MAFIAPACKET_GUI_ADDMSG);

	unsigned int color = 0;
	if (!pState->CheckNumber(2, color)) return false;

	Packet.Write<GChar*>((GChar*)msg);
	Packet.Write<unsigned int>(color);

	pClient->SendPacket(&Packet, PACKETPRIORITY_DEFAULT);

	return true;
}

static bool FunctionPlayerFadeScreen(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CMafiaClient* pClient;
	if (!pState->CheckClass(pServerManager->m_pServer->m_pManager->m_pNetMachineClass, 0, false, &pClient))
		return false;

	uint8_t fadeInOut;
	if (!pState->CheckNumber(1, fadeInOut)) return false;

	int time;
	if (!pState->CheckNumber(2, time)) return false;

	unsigned int color;
	if (!pState->CheckNumber(3, color)) return false;

	Packet Packet(MAFIAPACKET_GUI_FADE);
	Packet.Write<uint8_t>(fadeInOut);
	Packet.Write<int>(time);
	Packet.Write<unsigned int>(color);

	pClient->SendPacket(&Packet, PACKETPRIORITY_DEFAULT);

	return true;
}

static bool FunctionPlayerEnableMap(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CMafiaClient* pClient;
	if (!pState->CheckClass(pServerManager->m_pServer->m_pManager->m_pNetMachineClass, 0, false, &pClient))
		return false;

	bool state;
	if (!pState->CheckBoolean(1, state)) return false;

	Packet Packet(MAFIAPACKET_GUI_ENABLEMAP);
	Packet.Write<uint8_t>(state);

	pClient->SendPacket(&Packet, PACKETPRIORITY_DEFAULT);

	return true;
}

static bool FunctionPlayerAnnounce(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CMafiaClient* pClient;
	if (!pState->CheckClass(pServerManager->m_pServer->m_pManager->m_pNetMachineClass, 0, false, &pClient))
		return false;

	const GChar* msg = pState->CheckString(1);
	if (!msg) return false;

	Packet Packet(MAFIAPACKET_GUI_ANNOUNCE);

	float time = 0;
	if (!pState->CheckNumber(2, time)) return false;

	Packet.Write<GChar*>((GChar*)msg);
	Packet.Write<float>(time);

	pClient->SendPacket(&Packet, PACKETPRIORITY_DEFAULT);

	return true;
}

static bool FunctionPlayerCountdown(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CMafiaClient* pClient;
	if (!pState->CheckClass(pServerManager->m_pServer->m_pManager->m_pNetMachineClass, 0, false, &pClient))
		return false;

	Packet Packet(MAFIAPACKET_GUI_COUNTDOWN);

	uint8_t raceFlags;

	if (!pState->CheckNumber(1, raceFlags)) return false;

	Packet.Write<uint8_t>(raceFlags);

	pClient->SendPacket(&Packet, PACKETPRIORITY_DEFAULT);

	return true;
}

static bool FunctionCreateExplosion(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CVector3D vecPos(0.0f, 0.0f, 0.0f);
	if (!pState->CheckVector3D(0, vecPos))
		return false;
	float radius = 0;
	if (!pState->CheckNumber(1, radius)) return false;

	float force = 0;
	if (!pState->CheckNumber(2, force)) return false;

	Packet Packet(MAFIAPACKET_EXPLOSION);
	Packet.Write<CVector3D>(vecPos);
	Packet.Write<float>(radius);
	Packet.Write<float>(force);

	pServerManager->m_pServer->m_pNetSystem->SendEveryonePacket(&Packet);

	return true;
}

static bool FunctionCreateVehicle(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;

	const GChar* sModel = pState->CheckString(0);
	if (!sModel)
		return false;

	CVector3D vecPos(0.0f, 0.0f, 0.0f);
	if (!pState->CheckVector3D(1, vecPos))
		return false;

	float angle = 0.0;
	if (argc > 2 && !pState->CheckNumber(2, angle))
		return false;

	//_glogprintf(_gstr("HEADING: %f"), angle);

	//double twoPi = M_PI * 2.0;
	//while (angle < -twoPi)
	//	angle += twoPi;
	//while (angle > twoPi)
	//	angle -= twoPi;

	Strong<CServerVehicle> pServerVehicle;

	pServerVehicle = Strong<CServerVehicle>::New(pServerManager->Create(ELEMENT_VEHICLE));

	//angle = CVecTools::RadToDeg(angle);
	CVector3D vecRot = CVecTools::ComputeDirEuler(angle);

	//_glogprintf(_gstr("ROTATION: %f, %f, %f"), vecRot.x, vecRot.y, vecRot.z);

	pServerVehicle->SetModel(sModel);
	pServerVehicle->SetPosition(vecPos);
	pServerVehicle->SetRotation(vecRot);
	pServerVehicle->m_RotationFront = CVecTools::ComputeDirVector(CVecTools::RadToDeg(angle));
	pServerVehicle->m_RotationUp = { 0.0, 0.0, 0.0 };
	pServerVehicle->m_RotationRight = { 0.0, 0.0, 0.0 };
	//pServerVehicle->SetRotationMat(CVecTools::ComputeDirVector(CVecTools::RadToDeg(angle)), 0.0, 0.0);

	pServerVehicle->m_Health = 100.0f;
	pServerVehicle->m_EngineHealth = 100.0f;
	pServerVehicle->m_Fuel = 50.0f;
	pServerVehicle->m_SoundEnabled = true;
	pServerVehicle->m_Engine = false;
	pServerVehicle->m_Horn = false;
	pServerVehicle->m_Siren = false;
	pServerVehicle->m_Gear = 1;
	pServerVehicle->m_Lights = false;
	pServerVehicle->m_EngineRPM = 0;
	pServerVehicle->m_Accelerating = 0;
	pServerVehicle->m_Breakval = 0;
	pServerVehicle->m_Handbrake = 0;
	pServerVehicle->m_SpeedLimit = 1000.0f;
	pServerVehicle->m_Clutch = 1.0f;
	pServerVehicle->m_WheelAngle = 0;
	pServerVehicle->m_Velocity = CVector3D(0, 0, 0);
	pServerVehicle->m_RotVelocity = CVector3D(0, 0, 0);

	pServerVehicle->m_pResource = pState->GetResource();
	pServerManager->RegisterNetObject(pServerVehicle);
	pState->ReturnObject(pServerVehicle);
	return true;
}

static bool FunctionCreateDummyElement(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	
	CVector3D vecPos(0.0f, 0.0f, 0.0f);
	if (!pState->CheckVector3D(0, vecPos))
		return false;

	auto pElement = Strong<CServerDummy>::New(pServerManager->Create(ELEMENT_DUMMY));
	if (pElement == nullptr)
	{
		pState->Error(_gstr("Failed to create dummy element"));
		return false;
	}

	pElement->SetPosition(vecPos);
	pElement->m_pResource = pState->GetResource();
	pServerManager->RegisterNetObject(pElement);
	pState->ReturnObject(pElement);
	return true;
}

static bool FunctionCreateObject(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;

	const GChar* sModel = pState->CheckString(0);
	if (!sModel)
		return false;

	CVector3D vecPos(0.0f, 0.0f, 0.0f);
	if (!pState->CheckVector3D(1, vecPos))
		return false;

	CVector3D vecRot(0.0f, 0.0f, 0.0f);
	if (!pState->CheckVector3D(2, vecRot))
		return false;

	auto pElement = Strong<CServerObject>::New(pServerManager->Create(ELEMENT_OBJECT));
	if (pElement == nullptr)
	{
		pState->Error(_gstr("Failed to create object"));
		return false;
	}

	pElement->SetModel(sModel);
	pElement->SetPosition(vecPos);
	pElement->SetRotation(vecRot);
	pElement->m_pResource = pState->GetResource();
	pServerManager->RegisterNetObject(pElement);
	pState->ReturnObject(pElement);
	return true;
}

static bool FunctionCreateHuman(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	const GChar* sModel = pState->CheckString(0);
	if (!sModel)
		return false;
	CVector3D vecPos(0.0f, 0.0f, 0.0f);
	if (argc > 1 && !pState->CheckVector3D(1, vecPos))
		return false;
	float angle = 0;
	if (argc > 2 && !pState->CheckNumber(2, angle))
		return false;

	auto pPed = Strong<CServerHuman>::New(pServerManager->Create(ELEMENT_PED));
	if (pPed == nullptr)
	{
		pState->Error(_gstr("Failed to create civilian"));
		return false;
	}

	pPed->SetModel(sModel);
	pPed->SetPosition(vecPos);
	pPed->m_pResource = pState->GetResource();
	pServerManager->RegisterNetObject(pPed);
	pPed->OnSpawned();
	pState->ReturnObject(pPed);
	return true;
}

static bool FunctionCreatePlayer(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	const GChar* sModel = pState->CheckString(0);
	if (!sModel)
		return false;
	CVector3D vecPos(0.0f, 0.0f, 0.0f);
	if (!pState->CheckVector3D(1, vecPos))
		return false;
	float angle = 0;
	if (argc > 2 && !pState->CheckNumber(2, angle))
		return false;
	auto pServerPlayer = Strong<CServerPlayer>::New(pServerManager->Create(ELEMENT_PLAYER));
	if (pServerPlayer == nullptr)
	{
		pState->Error(_gstr("Failed to create player"));
		return false;
	}

	pServerPlayer->SetPosition(vecPos);
	pServerPlayer->SetModel(sModel);
	pServerPlayer->SetHeading(angle);
	pServerPlayer->m_pResource = pState->GetResource();
	pServerManager->RegisterNetObject(pServerPlayer);
	pServerPlayer->OnSpawned();
	pState->ReturnObject(pServerPlayer);
	return true;
}

static bool FunctionGetServerRcon(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnBoolean(pServerManager->m_pServer->m_bRCon);
	return true;
}

static bool FunctionGetServerRconPort(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnNumber(pServerManager->m_pServer->m_usRConPort);
	return true;
}

static bool FunctionGetServerListed(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnBoolean(pServerManager->m_pServer->m_bServerBrowser);
	return true;
}

static bool FunctionGetServerHttpServer(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnBoolean(pServerManager->m_pServer->m_bHttpServer);
	return true;
}

static bool FunctionGetServerHttpPort(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnNumber(pServerManager->m_pServer->m_usHTTPPort);
	return true;
}

static bool FunctionGetServerMinClientVersion(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	GChar szMinClientVersion[64];
	_gsnprintf(szMinClientVersion, ARRAY_COUNT(szMinClientVersion), _gstr("%u.%u.%u"), pServerManager->m_pServer->m_uiMinMajorVersion, pServerManager->m_pServer->m_uiMinMinorVersion, pServerManager->m_pServer->m_uiMinPatchVersion);
	pState->ReturnString(szMinClientVersion);
	return true;
}

static bool FunctionGetServerSyncInterval(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnNumber(pServerManager->m_pServer->m_usSyncInterval);
	return true;
}

static bool FunctionGetServerSyncMethod(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnNumber((uint32_t)pServerManager->m_pServer->m_SyncMethod);
	return true;
}

static bool FunctionGetServerDuplicateNames(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnBoolean(pServerManager->m_pServer->m_bDuplicateNames);
	return true;
}

static bool FunctionGetServerStreamInDistance(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnNumber(pServerManager->m_pServer->m_fStreamInDistance);
	return true;
}

static bool FunctionGetServerStreamOutDistance(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnNumber(pServerManager->m_pServer->m_fStreamOutDistance);
	return true;
}

static bool FunctionGetServerLogPath(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnString(pServerManager->m_pServer->m_szLogPath);
	return true;
}

static bool FunctionGetServerSyncLocalEntities(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnBoolean(pServerManager->m_pMafiaServer->m_bSyncLocalEntities);
	return true;
}

static bool FunctionGetServerCVar(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	const GChar* pszName = pState->CheckString(0);
	if (!pszName)
		return false;
	auto pVar = pServerManager->m_pServer->m_CVars.Find(pszName);
	if (pVar == nullptr)
	{
		pState->ReturnNull();
		return true;
	}
	pState->ReturnString(pVar->m_pszValue);
	return true;
}

static bool FunctionSetServerPassword(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	const GChar* pszPassword = pState->CheckString(0);
	if (!pszPassword)
		return false;
	if (pServerManager->m_pServer->m_Password.SetPassword(pszPassword))
	{
		pState->ReturnBoolean(true);
		return true;
	}
	return pState->Error(_gstr("Failed to change the server password."));
}

static bool FunctionPedGetHealth(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerHuman* pServerPed;
	if (!pState->GetThis(pServerManager->m_pServerHumanClass, &pServerPed))
		return false;
	pState->ReturnNumber(pServerPed->m_fHealth);
	return true;
}

static bool FunctionPedGetSeat(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerHuman* pServerPed;
	if (!pState->GetThis(pServerManager->m_pServerHumanClass, &pServerPed))
		return false;
	pState->ReturnNumber(pServerPed->m_nSeat);
	return true;
}

static bool FunctionPedSetHealth(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerHuman* pServerPed;
	if (!pState->GetThis(pServerManager->m_pServerHumanClass, &pServerPed))
		return false;

	float health = 0;
	if (pState->CheckNumber(0, health))
		return false;

	pServerPed->SetHealth(health);
	pState->ReturnNumber(health);
	return true;
}

static bool FunctionPedGetAnimationState(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerHuman* pServerPed;
	if (!pState->GetThis(pServerManager->m_pServerHumanClass, &pServerPed))
		return false;
	pState->ReturnNumber(pServerPed->m_AnimationState);
	return true;
}

static bool FunctionPedGetOccupiedVehicle(IScriptState* pState, int argc, void* pUser)
{
    CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
    CServerHuman* pServerPed;
    if (!pState->GetThis(pServerManager->m_pServerHumanClass, &pServerPed))
        return false;
    if(pServerPed->IsInVehicle())
		pState->ReturnObject(pServerManager->FromId(pServerPed->m_nVehicleNetworkIndex));
	else
		pState->ReturnNull();
    return true;
}

static bool FunctionPedIsEnteringExitingVehicle(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerHuman* pServerPed;
	if (!pState->GetThis(pServerManager->m_pServerHumanClass, &pServerPed))
		return false;
	pState->ReturnBoolean(pServerPed->m_EnteringExitingVehicle);
	return true;
}

static bool FunctionGetServerBindIP(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnString(pServerManager->m_pServer->m_szBindIP);
	return true;
}

template <int type>
static bool FunctionGetElements(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	size_t uiCount = 0;
	for (size_t i = 0; i < pServerManager->m_Objects.GetSize(); i++)
	{
		if (pServerManager->m_Objects.IsUsedAt(i))
		{
			auto pObject = pServerManager->m_Objects.GetAt(i);
			if (pObject != nullptr && pObject->IsType(type))
			{
				uiCount++;
			}
		}
	}
	CArgumentArray* pEntries = new CArgumentArray(uiCount);
	for (size_t i = 0; i < pServerManager->m_Objects.GetSize(); i++)
	{
		if (pServerManager->m_Objects.IsUsedAt(i))
		{
			auto pObject = pServerManager->m_Objects.GetAt(i);
			if (pObject != nullptr && pObject->IsType(type))
			{
				pEntries->AddObject(pObject);
			}
		}
	}
	pState->ReturnAndOwn(pEntries);
	return true;
}

template <int type>
static bool FunctionGetElementCount(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	size_t uiCount = 0;
	for (size_t i = 0; i < pServerManager->m_Objects.GetSize(); i++)
	{
		if (pServerManager->m_Objects.IsUsedAt(i))
		{
			auto pObject = pServerManager->m_Objects.GetAt(i);
			if (pObject != nullptr && pObject->IsType(type))
			{
				uiCount++;
			}
		}
	}
	pState->ReturnNumber(uiCount);
	return true;
}

void CMafiaServerManager::RegisterFunctions(CScripting* pScripting)
{
	auto pGameNamespace = pScripting->m_Global.AddNamespace(_gstr("mafia"));
	pGameNamespace->SetAlias(_gstr("game"));

	{
		pScripting->m_Global.RegisterFunction(_gstr("getElements"), _gstr(""), FunctionGetElements<ELEMENT_ELEMENT>, this);
		pScripting->m_Global.RegisterFunction(_gstr("getPeds"), _gstr(""), FunctionGetElements<ELEMENT_PED>, this);
		pScripting->m_Global.RegisterFunction(_gstr("getPlayers"), _gstr(""), FunctionGetElements<ELEMENT_PLAYER>, this);
		pScripting->m_Global.RegisterFunction(_gstr("getVehicles"), _gstr(""), FunctionGetElements<ELEMENT_VEHICLE>, this);
	}

	{
		pScripting->m_Global.RegisterFunction(_gstr("getElementCount"), _gstr(""), FunctionGetElementCount<ELEMENT_ELEMENT>, this);
		pScripting->m_Global.RegisterFunction(_gstr("getPedCount"), _gstr(""), FunctionGetElementCount<ELEMENT_PED>, this);
		pScripting->m_Global.RegisterFunction(_gstr("getPlayerCount"), _gstr(""), FunctionGetElementCount<ELEMENT_PLAYER>, this);
		pScripting->m_Global.RegisterFunction(_gstr("getVehicleCount"), _gstr(""), FunctionGetElementCount<ELEMENT_VEHICLE>, this);
	}

	{
		m_pServerEntityClass->AddProperty(this, _gstr("heading"), ARGUMENT_FLOAT, FunctionEntityGetHeading, FunctionEntitySetHeading);
		m_pServerEntityClass->AddProperty(this, _gstr("model"), ARGUMENT_STRING, FunctionEntityGetModel, FunctionEntitySetModel);
		m_pServerEntityClass->AddProperty(this, _gstr("modelIndex"), ARGUMENT_STRING, FunctionEntityGetModel, FunctionEntitySetModel); // For GTAC compatibility
		m_pServerEntityClass->AddProperty(this, _gstr("velocity"), ARGUMENT_VECTOR3D, FunctionEntityGetVelocity, FunctionEntitySetVelocity);
		m_pServerEntityClass->AddProperty(this, _gstr("turnVelocity"), ARGUMENT_VECTOR3D, FunctionEntityGetRotationVelocity, FunctionEntitySetRotationVelocity); // For GTAC compatibility
		m_pServerEntityClass->AddProperty(this, _gstr("rotationVelocity"), ARGUMENT_VECTOR3D, FunctionEntityGetRotationVelocity, FunctionEntitySetRotationVelocity);
	}

	{
		//m_pServerVehicleClass->AddProperty(this, _gstr("locked"), ARGUMENT_BOOLEAN, FunctionVehicleGetLocked, FunctionVehicleSetLocked);
		m_pServerVehicleClass->AddProperty(this, _gstr("siren"), ARGUMENT_BOOLEAN, FunctionVehicleGetSiren, FunctionVehicleSetSiren);
		m_pServerVehicleClass->AddProperty(this, _gstr("engine"), ARGUMENT_BOOLEAN, FunctionVehicleGetEngine, FunctionVehicleSetEngine);
		//m_pServerVehicleClass->AddProperty(this, _gstr("roof"), ARGUMENT_BOOLEAN, FunctionVehicleGetRoof, FunctionVehicleSetRoof);
		m_pServerVehicleClass->AddProperty(this, _gstr("lights"), ARGUMENT_BOOLEAN, FunctionVehicleGetLights, FunctionVehicleSetLights);
		m_pServerVehicleClass->AddProperty(this, _gstr("fuel"), ARGUMENT_FLOAT, FunctionVehicleGetFuel, FunctionVehicleSetFuel);
		m_pServerVehicleClass->AddProperty(this, _gstr("wheelAngle"), ARGUMENT_FLOAT, FunctionVehicleGetWheelAngle, FunctionVehicleSetWheelAngle);
		m_pServerVehicleClass->AddProperty(this, _gstr("speedLimit"), ARGUMENT_FLOAT, FunctionVehicleGetSpeedLimit, FunctionVehicleSetSpeedLimit);
		m_pServerVehicleClass->AddProperty(this, _gstr("engineRPM"), ARGUMENT_FLOAT, FunctionVehicleGetEngineRPM, FunctionVehicleSetEngineRPM);
		//m_pServerVehicleClass->AddProperty(this, _gstr("speed"), ARGUMENT_FLOAT, FunctionVehicleGetSpeed, FunctionVehicleSetSpeed);
		//m_pServerVehicleClass->AddProperty(this, _gstr("turnVelocity"), ARGUMENT_VECTOR3D, FunctionVehicleGetRotationVelocity, FunctionVehicleSetRotationVelocity);
		//m_pServerVehicleClass->AddProperty(this, _gstr("velocity"), ARGUMENT_VECTOR3D, FunctionVehicleGetVelocity, FunctionVehicleSetVelocity);
		m_pServerVehicleClass->AddProperty(this, _gstr("engineHealth"), ARGUMENT_FLOAT, FunctionVehicleGetEngineHealth, FunctionVehicleSetEngineHealth);
		m_pServerVehicleClass->RegisterFunction(_gstr("fix"), _gstr("t"), FunctionVehicleFix, this);
		m_pServerVehicleClass->RegisterFunction(_gstr("getOccupant"), _gstr("ti"), FunctionVehicleGetOccupant, this);
		m_pServerVehicleClass->RegisterFunction(_gstr("getOccupants"), _gstr("t"), FunctionVehicleGetOccupants, this);
		m_pServerVehicleClass->RegisterFunction(_gstr("setEngineState"), _gstr("tbb"), FunctionVehicleSetEngineDetailed, this);
		m_pServerVehicleClass->AddProperty(this, _gstr("gear"), ARGUMENT_INTEGER, FunctionVehicleGetGear, FunctionVehicleSetGear);
	}

	{
		m_pServerHumanClass->AddProperty(this, _gstr("skin"), ARGUMENT_STRING, FunctionPedGetSkin, FunctionPedSetSkin);
		m_pServerHumanClass->AddProperty(this, _gstr("vehicle"), ARGUMENT_OBJECT, FunctionPedGetOccupiedVehicle);
		m_pServerHumanClass->AddProperty(this, _gstr("seat"), ARGUMENT_INTEGER, FunctionPedGetSeat);
		m_pServerHumanClass->AddProperty(this, _gstr("health"), ARGUMENT_FLOAT, FunctionPedGetHealth);
		m_pServerHumanClass->AddProperty(this, _gstr("animationState"), ARGUMENT_INTEGER, FunctionPedGetAnimationState);
		//m_pServerPedClass->AddProperty(this, _gstr("weaponAmmunition"), ARGUMENT_INTEGER, FunctionPedGetWeaponAmmunition);
		//m_pServerPedClass->AddProperty(this, _gstr("weaponClipAmmunition"), ARGUMENT_INTEGER, FunctionPedGetWeaponClipAmmunition);
		//m_pServerPedClass->AddProperty(this, _gstr("weaponState"), ARGUMENT_INTEGER, FunctionPedGetWeaponState);
		m_pServerHumanClass->AddProperty(this, _gstr("isEnteringVehicle"), ARGUMENT_BOOLEAN, FunctionPedIsEnteringExitingVehicle);
		m_pServerHumanClass->AddProperty(this, _gstr("isExitingVehicle"), ARGUMENT_BOOLEAN, FunctionPedIsEnteringExitingVehicle);
		m_pServerHumanClass->RegisterFunction(_gstr("giveWeapon"), _gstr("tiii"), FunctionHumanGiveWeapon, this);
	}

	{
		auto pHUDNamespace = pGameNamespace->AddNamespace(_gstr("hud"));
		pHUDNamespace->RegisterFunction(_gstr("message"), _gstr("xsi"), FunctionPlayerHudMsg, this);
		pHUDNamespace->RegisterFunction(_gstr("enableMap"), _gstr("xb"), FunctionPlayerEnableMap, this);
		pHUDNamespace->RegisterFunction(_gstr("announce"), _gstr("xsf"), FunctionPlayerAnnounce, this);
		pHUDNamespace->RegisterFunction(_gstr("showCountdown"), _gstr("xi"), FunctionPlayerCountdown, this);
	}

	pScripting->m_Global.RegisterFunction(_gstr("spawnPlayer"), _gstr("xvfs"), FunctionSpawnPlayer, this);

	pGameNamespace->AddProperty(this, _gstr("mapName"), ARGUMENT_STRING, FunctionGameGetLevel);
	pGameNamespace->RegisterFunction(_gstr("changeMap"), _gstr("s"), FunctionGameSetLevel, this);

	pGameNamespace->RegisterFunction(_gstr("createExplosion"), _gstr("vff"), FunctionCreateExplosion, this);
	pGameNamespace->RegisterFunction(_gstr("createVehicle"), _gstr("sv|f"), FunctionCreateVehicle, this);
	pGameNamespace->RegisterFunction(_gstr("createPlayer"), _gstr("sv|f"), FunctionCreatePlayer, this);
	pGameNamespace->RegisterFunction(_gstr("createPed"), _gstr("sv|f"), FunctionCreateHuman, this);
	pGameNamespace->RegisterFunction(_gstr("createDummyElement"), _gstr("v"), FunctionCreateDummyElement, this);
	pGameNamespace->RegisterFunction(_gstr("createObject"), _gstr("svv"), FunctionCreateObject, this);
	pGameNamespace->RegisterFunction(_gstr("fadeScreen"), _gstr("xbf|i"), FunctionPlayerFadeScreen, this);

	{
		Galactic3D::ReflectedNamespace* pServerNamespace = pScripting->m_Global.AddNamespace(_gstr("server"));

		pServerNamespace->AddProperty(this, _gstr("rcon"), ARGUMENT_BOOLEAN, FunctionGetServerRcon);
		pServerNamespace->AddProperty(this, _gstr("rconPort"), ARGUMENT_INTEGER, FunctionGetServerRconPort);
		pServerNamespace->AddProperty(this, _gstr("listed"), ARGUMENT_BOOLEAN, FunctionGetServerListed);
		pServerNamespace->AddProperty(this, _gstr("httpServer"), ARGUMENT_BOOLEAN, FunctionGetServerHttpServer);
		pServerNamespace->AddProperty(this, _gstr("httpPort"), ARGUMENT_INTEGER, FunctionGetServerHttpPort);
		pServerNamespace->AddProperty(this, _gstr("minClientVersion"), ARGUMENT_STRING, FunctionGetServerMinClientVersion);
		pServerNamespace->AddProperty(this, _gstr("syncInterval"), ARGUMENT_INTEGER, FunctionGetServerSyncInterval);
		pServerNamespace->AddProperty(this, _gstr("syncMethod"), ARGUMENT_INTEGER, FunctionGetServerSyncMethod);
		pServerNamespace->AddProperty(this, _gstr("duplicateNames"), ARGUMENT_BOOLEAN, FunctionGetServerDuplicateNames);
		pServerNamespace->AddProperty(this, _gstr("streamInDistance"), ARGUMENT_FLOAT, FunctionGetServerStreamInDistance);
		pServerNamespace->AddProperty(this, _gstr("streamOutDistance"), ARGUMENT_FLOAT, FunctionGetServerStreamOutDistance);
		pServerNamespace->AddProperty(this, _gstr("logPath"), ARGUMENT_STRING, FunctionGetServerLogPath);
		pServerNamespace->AddProperty(this, _gstr("syncLocalEntities"), ARGUMENT_BOOLEAN, FunctionGetServerSyncLocalEntities);
		pServerNamespace->AddProperty(this, _gstr("bindIP"), ARGUMENT_STRING, FunctionGetServerBindIP);
		pServerNamespace->RegisterFunction(_gstr("getCVar"), _gstr("s"), FunctionGetServerCVar, this);
		pServerNamespace->RegisterFunction(_gstr("setPassword"), _gstr("s"), FunctionSetServerPassword, this);
		
	}
}
