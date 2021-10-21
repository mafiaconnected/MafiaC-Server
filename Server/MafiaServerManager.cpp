
#include "pch.h"
#include "Server.h"
#include "MafiaServerManager.h"
#include "Utils/VectorTools.h"

CMafiaClient::CMafiaClient(CNetObjectMgr* pServer) :
	CNetMachine(pServer)
{
}

void CMafiaClient::SpawnPlayer(const CVector3D& vecPos, float fRotation, const GChar* modelName)
{
	DespawnPlayer();

	auto pPlayer = Strong<CServerPlayer>::New(static_cast<CServerPlayer*>(m_pNetObjectMgr->Create(ELEMENT_PLAYER)));

	//_glogprintf(_gstr("SpawnPlayer - Player model: %s"), modelName);

	_gstrcpy_s(pPlayer->m_szModel, ARRAY_COUNT(pPlayer->m_szModel), modelName);
	pPlayer->SetSyncer(m_nIndex,true);
	pPlayer->SetHeading(fRotation);
	pPlayer->SetPosition(vecPos);
	pPlayer->SetRotation(CVecTools::ComputeDirEuler(fRotation));

	if (!m_pNetObjectMgr->RegisterObject(pPlayer))
		return;

	SetPlayer(pPlayer);
	pPlayer->OnSpawned();
}

CMafiaServerManager::CMafiaServerManager(Context* pContext, CServer* pServer) :
	CServerManager(pContext, pServer)
{
	m_Games.Register(_gstr("mafia:one"), GAME_MAFIA_ONE);
	m_Games.Register(_gstr("mafia:two"), GAME_MAFIA_TWO);
	m_Games.Register(_gstr("mafia:three"), GAME_MAFIA_THREE);
	m_Games.Register(_gstr("mafia:one_de"), GAME_MAFIA_ONE_DE);

	m_pOnPedEnteredVehicleEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedEnteredVehicle"), _gstr("Called when a ped is finished entering a vehicle."), 3);
	m_pOnPedExitedVehicleEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedExitedVehicle"), _gstr("Called when a ped has finished exiting a vehicle."), 3);
	m_pOnPedEnteringVehicleEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedEnteringVehicle"), _gstr("Called when a ped is started entering a vehicle."), 3);
	m_pOnPedExitingVehicleEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedExitingVehicle"), _gstr("Called when a ped has started exiting a vehicle."), 3);
	m_pOnPedDeathEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedDeath"), _gstr("Called when a ped dies."), 4);
	m_pOnPedSpawnEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedSpawn"), _gstr("Called when a ped is spawned."), 1);
	m_pOnPedFallEventType = m_pServer->m_ResourceMgr.m_pEventHandlers->CreateEventType(_gstr("OnPedFall"), _gstr("Called when a ped falls."), 1);

	auto pMafia = m_pServer->m_ResourceMgr.m_pScripting->m_Global.AddNamespace(_gstr("mafia"));
	pMafia->SetAlias(_gstr("game"));

	m_pServerEntityClass = pMafia->NewClass(_gstr("Entity"), m_pNetObjectClass);
	m_pServerHumanClass = pMafia->NewClass(_gstr("Ped"), m_pServerEntityClass);
	m_pServerPlayerClass = pMafia->NewClass(_gstr("Player"), m_pServerHumanClass);
	m_pServerVehicleClass = pMafia->NewClass(_gstr("Vehicle"), m_pServerEntityClass);

	auto pDefineHandlers = m_pServer->m_ResourceMgr.m_pDefineHandlers;

	pDefineHandlers->Define(_gstr("SERVER_VERSION_MAJOR"), MAFIAC_SERVER_MAJOR);
	pDefineHandlers->Define(_gstr("SERVER_VERSION_MINOR"), MAFIAC_SERVER_MINOR);
	pDefineHandlers->Define(_gstr("SERVER_VERSION_PATCH"), MAFIAC_SERVER_PATCH);

	pDefineHandlers->Define(_gstr("GAME_UNKNOWN"), GAME_UNKNOWN);
	pDefineHandlers->Define(_gstr("GAME_MAFIA_ONE"), GAME_MAFIA_ONE);
	pDefineHandlers->Define(_gstr("GAME_MAFIA_TWO"), GAME_MAFIA_TWO);
	pDefineHandlers->Define(_gstr("GAME_MAFIA_THREE"), GAME_MAFIA_THREE);
	pDefineHandlers->Define(_gstr("GAME_MAFIA_ONE_DE"), GAME_MAFIA_ONE_DE);

	pDefineHandlers->Define(_gstr("ELEMENT_ELEMENT"), ELEMENT_ELEMENT);
	pDefineHandlers->Define(_gstr("ELEMENT_ENTITY"), ELEMENT_ENTITY);
	pDefineHandlers->Define(_gstr("ELEMENT_PED"), ELEMENT_PED);
	pDefineHandlers->Define(_gstr("ELEMENT_PLAYER"), ELEMENT_PLAYER);
	pDefineHandlers->Define(_gstr("ELEMENT_VEHICLE"), ELEMENT_VEHICLE);

	RegisterFunctions(m_pServer->m_ResourceMgr.m_pScripting);
}

CNetObject* CMafiaServerManager::Create(int32_t nType)
{
	CServerHuman* human;
	CServerVehicle* vehicle;
	CServerPlayer* player;

	switch (nType)
	{
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

static bool FunctionEntityGetModel(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass,&pServerEntity))
		return false;

	UTF16String model(true, pServerEntity->m_szModel);

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

static bool FunctionEntityGetStreamInDistance(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass, &pServerEntity))
		return false;
	pState->ReturnNumber(pServerEntity->GetStreamInDistance());
	return true;
}

static bool FunctionEntitySetStreamInDistance(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass, &pServerEntity))
		return false;
	float fStreamInDistance;
	if (!pState->CheckNumber(0, fStreamInDistance))
		return false;
	pServerEntity->SetStreamInDistance(fStreamInDistance);
	return true;
}

static bool FunctionEntityGetStreamOutDistance(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass, &pServerEntity))
		return false;
	pState->ReturnNumber(pServerEntity->GetStreamOutDistance());
	return true;
}

static bool FunctionEntitySetStreamOutDistance(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerEntity* pServerEntity;
	if (!pState->GetThis(pServerManager->m_pServerEntityClass, &pServerEntity))
		return false;
	float fStreamOutDistance;
	if (!pState->CheckNumber(0, fStreamOutDistance))
		return false;
	pServerEntity->SetStreamOutDistance(fStreamOutDistance);
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
	pServerVehicle->SetEngine(bEngine);
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

static bool FunctionHumanGiveWeapon(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CServerHuman* pServerHuman;
	if (!pState->GetThis(pServerManager->m_pServerHumanClass, &pServerHuman))
		return false;

	unsigned short weaponId;
	if (!pState->CheckNumber(0, weaponId))
		return false;

	unsigned short ammo1 = 999;
	if (!pState->CheckNumber(1, ammo1))
		return false;

	unsigned short ammo2 = 999;
	if (!pState->CheckNumber(2, ammo2))
		return false;

	pServerHuman->GiveWeapon(weaponId, ammo1, ammo2);
	return true;
}

static bool FunctionGameSetLevel(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;

	const GChar* level = pState->CheckString(0);

	pServerManager->m_pServer->SetMapName(level);

	return true;
}

static bool FunctionGameGetLevel(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;

	pState->ReturnString(pServerManager->m_pServer->GetMapName());

	return true;
}

static bool FunctionSpawnPlayer(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	CMafiaClient* pClient;
	if (!pState->CheckClass(pServerManager->m_pServer->m_pManager->m_pNetMachineClass, 0, false, &pClient))
		return false;

	const GChar* sModel = pState->CheckString(1);

	CVector3D vecPos;
	if (!pState->CheckVector3D(2, vecPos))
		return false;

	float fRotation = 0.0f;
	if (argc > 2 && !pState->CheckNumber(3, fRotation))
		return false;

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

	float angle = 0;

	if (argc > 2 && !pState->CheckNumber(2, angle))
		return false;

	Strong<CServerVehicle> pServerVehicle;

	pServerVehicle = Strong<CServerVehicle>::New(pServerManager->Create(ELEMENT_VEHICLE));

	pServerVehicle->SetModel(sModel);
	pServerVehicle->SetPosition(vecPos);
	pServerVehicle->SetHeading(angle);

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

	pServerVehicle->m_pResource = pState->GetResource();
	pServerManager->RegisterObject(pServerVehicle);
	pServerVehicle->SetPosition(vecPos);
	pState->ReturnObject(pServerVehicle);
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
	pServerManager->RegisterObject(pPed);
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
	pServerManager->RegisterObject(pServerPlayer);
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

static bool FunctionGetServerPickupStreamInDistance(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnNumber(pServerManager->m_pServer->m_fPickupStreamInDistance);
	return true;
}

static bool FunctionGetServerPickupStreamOutDistance(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnNumber(pServerManager->m_pServer->m_fPickupStreamOutDistance);
	return true;
}

static bool FunctionGetServerLogPath(IScriptState* pState, int argc, void* pUser)
{
	CMafiaServerManager* pServerManager = (CMafiaServerManager*)pUser;
	pState->ReturnString(pServerManager->m_pServer->m_szLogPath);
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
	pState->ReturnNumber(pServerPed->m_ucSeat);
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

void CMafiaServerManager::RegisterFunctions(CScripting* pScripting)
{
	auto pGameNamespace = pScripting->m_Global.AddNamespace(_gstr("mafia"));
	pGameNamespace->SetAlias(_gstr("game"));

	//{
	//	m_pNetObjectTransformableClass->AddProperty(this, _gstr("streamInDistance"), ARGUMENT_FLOAT, FunctionEntityGetStreamInDistance, FunctionEntitySetStreamInDistance);
	//	m_pNetObjectTransformableClass->AddProperty(this, _gstr("streamOutDistance"), ARGUMENT_FLOAT, FunctionEntityGetStreamOutDistance, FunctionEntitySetStreamOutDistance);
	//}

	//{
	//	m_pServerEntityClass->AddProperty(this, _gstr("velocity"), ARGUMENT_VECTOR3D, FunctionEntityGetVelocity, FunctionEntitySetVelocity);
	//	m_pServerEntityClass->AddProperty(this, _gstr("turnVelocity"), ARGUMENT_VECTOR3D, FunctionEntityGetTurnVelocity, FunctionEntitySetTurnVelocity);
	//}

	{
		m_pServerVehicleClass->AddProperty(this, _gstr("model"), ARGUMENT_STRING, FunctionEntityGetModel, FunctionEntitySetModel);
		m_pServerVehicleClass->AddProperty(this, _gstr("heading"), ARGUMENT_FLOAT, FunctionEntityGetHeading, FunctionEntitySetHeading);

		m_pServerVehicleClass->AddProperty(this, _gstr("locked"), ARGUMENT_BOOLEAN, FunctionVehicleGetLocked, FunctionVehicleSetLocked);
		m_pServerVehicleClass->AddProperty(this, _gstr("siren"), ARGUMENT_BOOLEAN, FunctionVehicleGetSiren, FunctionVehicleSetSiren);
		m_pServerVehicleClass->AddProperty(this, _gstr("engine"), ARGUMENT_BOOLEAN, FunctionVehicleGetEngine, FunctionVehicleSetEngine);
		m_pServerVehicleClass->AddProperty(this, _gstr("roof"), ARGUMENT_BOOLEAN, FunctionVehicleGetRoof, FunctionVehicleSetRoof);
		m_pServerVehicleClass->AddProperty(this, _gstr("lights"), ARGUMENT_BOOLEAN, FunctionVehicleGetLights, FunctionVehicleSetLights);

		m_pServerVehicleClass->RegisterFunction(_gstr("fix"), _gstr("t"), FunctionVehicleFix, this);
		m_pServerVehicleClass->RegisterFunction(_gstr("getOccupant"), _gstr("ti"), FunctionVehicleGetOccupant, this);
		m_pServerVehicleClass->RegisterFunction(_gstr("getOccupants"), _gstr("t"), FunctionVehicleGetOccupants, this);
	}

	{
		m_pServerHumanClass->AddProperty(this, _gstr("model"), ARGUMENT_STRING, FunctionEntityGetModel, FunctionEntitySetModel);
		m_pServerHumanClass->AddProperty(this, _gstr("heading"), ARGUMENT_FLOAT, FunctionEntityGetHeading, FunctionEntitySetHeading);

		m_pServerHumanClass->AddProperty(this, _gstr("vehicle"), ARGUMENT_OBJECT, FunctionPedGetOccupiedVehicle);
		m_pServerHumanClass->AddProperty(this, _gstr("seat"), ARGUMENT_INTEGER, FunctionPedGetSeat);
		m_pServerHumanClass->AddProperty(this, _gstr("health"), ARGUMENT_FLOAT, FunctionPedGetHealth);
		m_pServerHumanClass->AddProperty(this, _gstr("animationState"), ARGUMENT_INTEGER, FunctionPedGetAnimationState);
		//m_pServerPedClass->AddProperty(this, _gstr("weaponAmmunition"), ARGUMENT_INTEGER, FunctionPedGetWeaponAmmunition);
		//m_pServerPedClass->AddProperty(this, _gstr("weaponClipAmmunition"), ARGUMENT_INTEGER, FunctionPedGetWeaponClipAmmunition);
		//m_pServerPedClass->AddProperty(this, _gstr("weaponState"), ARGUMENT_INTEGER, FunctionPedGetWeaponState);
		//m_pServerPedClass->AddProperty(this, _gstr("isEnteringVehicle"), ARGUMENT_BOOLEAN, FunctionPedIsEnteringVehicle);
		//m_pServerPedClass->AddProperty(this, _gstr("isExitingVehicle"), ARGUMENT_BOOLEAN, FunctionPedIsExitingVehicle);

		m_pServerHumanClass->RegisterFunction(_gstr("giveWeapon"), _gstr("ti|ii"), FunctionHumanGiveWeapon, this);
	}

	{
		auto pHUDNamespace = pGameNamespace->AddNamespace(_gstr("hud"));
		m_pServerPlayerClass->RegisterFunction(_gstr("message"), _gstr("xsi"), FunctionPlayerHudMsg, this);
		m_pServerPlayerClass->RegisterFunction(_gstr("enableMap"), _gstr("xb"), FunctionPlayerEnableMap, this);
		m_pServerPlayerClass->RegisterFunction(_gstr("announce"), _gstr("xsf"), FunctionPlayerAnnounce, this);
		m_pServerPlayerClass->RegisterFunction(_gstr("showCountdown"), _gstr("xi"), FunctionPlayerCountdown, this);
	}

	pScripting->m_Global.RegisterFunction(_gstr("spawnPlayer"), _gstr("xsv|f"), FunctionSpawnPlayer, this);

	pGameNamespace->AddProperty(this, _gstr("mapName"), ARGUMENT_STRING, FunctionGameGetLevel);
	pGameNamespace->RegisterFunction(_gstr("changeMap"), _gstr("s"), FunctionGameSetLevel, this);

	pGameNamespace->RegisterFunction(_gstr("createExplosion"), _gstr("vff"), FunctionCreateExplosion, this);
	pGameNamespace->RegisterFunction(_gstr("createVehicle"), _gstr("sv|f"), FunctionCreateVehicle, this);
	pGameNamespace->RegisterFunction(_gstr("createPlayer"), _gstr("sv|f"), FunctionCreatePlayer, this);
	pGameNamespace->RegisterFunction(_gstr("createPed"), _gstr("sv|f"), FunctionCreateHuman, this);
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
		pServerNamespace->AddProperty(this, _gstr("pickupStreamInDistance"), ARGUMENT_FLOAT, FunctionGetServerPickupStreamInDistance);
		pServerNamespace->AddProperty(this, _gstr("pickupStreamOutDistance"), ARGUMENT_FLOAT, FunctionGetServerPickupStreamOutDistance);
		pServerNamespace->AddProperty(this, _gstr("logPath"), ARGUMENT_STRING, FunctionGetServerLogPath);

		pServerNamespace->RegisterFunction(_gstr("getCVar"), _gstr("s"), FunctionGetServerCVar, this);
		//pServerNS->RegisterFunction("setCVar", "s*", FunctionSetServerCVar, this);
	}
}
