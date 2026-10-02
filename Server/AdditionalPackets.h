#pragma once

#pragma pack(push,1)

#include "ServerVersion.h"

#define NETGAME_CURRENT_VERSION 8

#include <Multiplayer/Packets.h>

enum eGame
{
	GAME_UNKNOWN,

	// GTA Connected
	GAME_GTA_III,
	GAME_GTA_VC,
	GAME_GTA_SA,
	//GAME_GTA_UG,
	GAME_GTA_IV = 5,
	GAME_BULLY = 7,

	// Mafia Connected
	GAME_MAFIA_ONE = 10, // Start at 10 since GTAC occupies the first lot to avoid confusion
	GAME_MAFIA_TWO,
	GAME_MAFIA_THREE,
	GAME_MAFIA_ONE_DE,
	GAME_COUNT,
};

// Start packets from PACKET_CUSTOM
enum eMafiaPacket : unsigned int
{
	MAFIAPACKET_NONE = PACKET_CUSTOM,
	MAFIAPACKET_CHANGEMAP,
	MAFIAPACKET_EXPLOSION,
	MAFIAPACKET_GUI_ADDMSG,
	MAFIAPACKET_GUI_ANNOUNCE,
	MAFIAPACKET_GUI_ENABLEMAP,
	MAFIAPACKET_GUI_COUNTDOWN,
	MAFIAPACKET_GUI_FADE,
	MAFIAPACKET_HUMAN_CREATE,
	MAFIAPACKET_HUMAN_DIE,
	MAFIAPACKET_HUMAN_SHOOT,
	MAFIAPACKET_HUMAN_THROWGRENADE,
	MAFIAPACKET_HUMAN_RELOAD,
	MAFIAPACKET_HUMAN_JUMP,
	MAFIAPACKET_HUMAN_HOLSTER,
	MAFIAPACKET_HUMAN_ADDWEAP,
	MAFIAPACKET_HUMAN_REMWEAP,
	MAFIAPACKET_HUMAN_DROPWEAP,
	MAFIAPACKET_HUMAN_CHANGEWEAP,
	MAFIAPACKET_HUMAN_SETMODEL,
	MAFIAPACKET_HUMAN_SETHEALTH,
	MAFIAPACKET_HUMAN_ENTERINGVEHICLE,
	MAFIAPACKET_HUMAN_ENTEREDVEHICLE,
	MAFIAPACKET_HUMAN_EXITINGVEHICLE,
	MAFIAPACKET_HUMAN_EXITEDVEHICLE,
	MAFIAPACKET_HUMAN_JACKVEHICLE,
	MAFIAPACKET_HUMAN_HIT,
	MAFIAPACKET_HUMAN_USINGACTOR,
	MAFIAPACKET_VEHICLE_CREATE,
	MAFIAPACKET_VEHICLE_FIX,
	MAFIAPACKET_VEHICLE_EXPLODE,
	MAFIAPACKET_VEHICLE_SETLOCKED,
	MAFIAPACKET_VEHICLE_SETENGINE,
	MAFIAPACKET_VEHICLE_SETSIREN,
	MAFIAPACKET_VEHICLE_SETROOF,
	MAFIAPACKET_VEHICLE_SETLIGHTS,
	MAFIAPACKET_VEHICLE_SETDAMAGE,
	MAFIAPACKET_VEHICLE_SETSPEEDLIMIT,
	MAFIAPACKET_VEHICLE_SETFUEL,
	MAFIAPACKET_VEHICLE_SETENGINERPM,
	MAFIAPACKET_VEHICLE_SETWHEELANGLE,
	MAFIAPACKET_VEHICLE_SETENGINEHEALTH,
	MAFIAPACKET_VEHICLE_SETGEAR,
	MAFIAPACKET_ELEMENT_UPDATE_ID,
	MAFIAPACKET_ELEMENT_REMOVE,
	MAFIAPACKET_PLAYER_SETPOSITION,
	MAFIAPACKET_PEER_CREATECAR,
	MAFIAPACKET_PEER_CREATECIVILIAN,
	MAFIAPACKET_PEER_IDENTIFY,
	MAFIAPACKET_PEER_CREATEACTOR,

	// Mafia 1 vehicle enter/exit round trip. The syncer of a ped asks (REQUEST) instead of just doing it; the server
	// checks it, records the seat and answers everyone, the requester included, with USEVEHICLE. Every client
	// (requester too) starts the enter/exit from that, so they all run it at the same point in the same order.
	MAFIAPACKET_HUMAN_USEVEHICLE_REQUEST,
	MAFIAPACKET_HUMAN_USEVEHICLE,

	MAFIAPACKET_VEHICLE_SETHEALTH,
};

struct tEntityCreatePacket
{
	CVector3D position;
	CVector3D positionRel;
	CVector3D rotation;
	CVector3D rotationRel;
};

struct tEntitySyncPacket
{
	CVector3D position;
	CVector3D positionRel;
	CVector3D rotation;
	CVector3D rotationRel;
};

struct tHumanCreatePacket
{
	float health;
	int32_t vehicleNetworkIndex;
	int8_t seat;
	bool isCrouching;
	bool isAiming;
	bool isShooting;
	uint32_t animStateLocal;
	bool isInAnimWithCarLocal;
	uint32_t animationState;
	bool isInAnimWithCar;
	float inCarRotation;
	int32_t animStopTime;
	int16_t weaponId;
};

struct tHumanSyncPacket
{
	float health;
	int32_t vehicleNetworkIndex;
	int8_t seat;
	bool isCrouching;
	bool isAiming;
	bool isShooting;
	uint32_t animStateLocal;
	bool isInAnimWithCarLocal;
	uint32_t animationState;
	bool isInAnimWithCar;
	float inCarRotation;
	int32_t animStopTime;
	int16_t weaponId;
	CVector3D camera;
	// Where the human is aiming/firing at (the target Do_Shoot was last given). With inCarRotation (the aim within
	// the vehicle) it is what the game needs to replay in-vehicle aiming and shooting on the other clients.
	CVector3D aimVector;
};

struct tVehicleCreatePacket
{
	CVector3D rotationFront;
	CVector3D rotationUp;
	CVector3D rotationRight;
	float health;
	float engineHealth;
	float fuel;
	bool sound;
	bool engineOn;
	bool horn;
	bool siren;
	bool lights;
	bool roof; // Up (or the model has none)
	int32_t gear;
	float rpm;
	float accel;
	float brake;
	float handBrake;
	float speedLimit;
	float clutch;
	float wheelAngle;
	CVector3D speed;
	CVector3D rotSpeed;
};

struct tVehicleSyncPacket
{
	CVector3D rotationFront;
	CVector3D rotationUp;
	CVector3D rotationRight;
	CQuaternion quatRot;
	float health;
	float engineHealth;
	float fuel;
	bool sound;
	bool engineOn;
	bool horn;
	bool siren;
	bool lights;
	bool roof; // Up (or the model has none)
	int32_t gear;
	float rpm;
	float accel;
	float brake;
	float handBrake;
	float speedLimit;
	float clutch;
	float wheelAngle;
	CVector3D speed;
	CVector3D rotSpeed;
};

/*
	Vehicle damage (Mafia 1), as one blob: the "damage" script property, MAFIAPACKET_VEHICLE_SETDAMAGE (client to server
	from the vehicle's syncer, server to clients) and an optional uint32 size + blob after tVehicleCreatePacket. The
	client builds and applies it from the game's own savegame snapshot of the car (MafiaSDK VehicleState); the server
	only stores it and reads this header.

	tVehicleDamageHeader, then lightCount * VEHICLEDAMAGE_LIGHT_SIZE bytes of lights, partBytes of deformable parts
	(deformation, glass, fallen off parts) and wheelCount * VEHICLEDAMAGE_WHEEL_SIZE bytes of wheel damage.
*/
#define VEHICLEDAMAGE_MAGIC 0x4456434D // "MCVD"
#define VEHICLEDAMAGE_VERSION 1
#define VEHICLEDAMAGE_MAX_SIZE 65536
#define VEHICLEDAMAGE_LIGHT_SIZE 12
#define VEHICLEDAMAGE_WHEEL_SIZE 12

struct tVehicleDamageHeader
{
	uint32_t magic;
	uint16_t version;
	uint16_t gameStateVersion;
	char model[32]; // Lowercase. Only ever applied to a vehicle of the same model.
	float health;
	float engineHealth;
	uint32_t lightCount;
	uint32_t partCount;
	uint32_t partBytes;
	uint32_t wheelCount;
};

enum eElementCreatedBy : uint8_t
{
	ELEMENTCREATEDBY_USER,
	ELEMENTCREATEDBY_POPULATION,
	ELEMENTCREATEDBY_MISSION,
	ELEMENTCREATEDBY_PARKED,
	ELEMENTCREATEDBY_SPECIAL,
};

#pragma pack(pop)