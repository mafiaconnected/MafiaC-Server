#pragma once

#pragma pack(push,1)

#include "ServerVersion.h"

#define NETGAME_CURRENT_VERSION 7

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

	// tVehicleStatePacket after the vehicle id. From a vehicle's syncer when its state changes, and from the server to
	// the clients that have the vehicle when the server's copy changes.
	MAFIAPACKET_VEHICLE_STATE,
	// Sent by a ped's syncer when the game puts the ped into its falling state. Given a fixed id instead of the next
	// in sequence because the client and server enums don't list the same packets on every branch.
	MAFIAPACKET_HUMAN_FALL = MAFIAPACKET_NONE + 0x100,

	// Server to the clients that have the ped: put it straight into a vehicle seat (ped id, vehicle id, int8 seat), or out of
	// whatever vehicle it's in (ped id)
	MAFIAPACKET_HUMAN_WARPINTOVEHICLE = MAFIAPACKET_NONE + 0x101,
	MAFIAPACKET_HUMAN_REMOVEFROMVEHICLE = MAFIAPACKET_NONE + 0x102,
	// tVehicleMafia2Packet after the vehicle id. Mafia 2 vehicles' looks and extra lights, from the server when a script
	// changes them and from the vehicle's syncer when its driver does.
	MAFIAPACKET_VEHICLE_MAFIA2 = MAFIAPACKET_NONE + 0x103,
	// Server to the clients that have the ped: play an animation on it (ped id, string name, bool repeat), or stop it (ped id)
	MAFIAPACKET_HUMAN_PLAYANIMATION = MAFIAPACKET_NONE + 0x104,
	MAFIAPACKET_HUMAN_STOPANIMATION = MAFIAPACKET_NONE + 0x105,
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

// Only what changes all the time while driving. The rest is tVehicleStatePacket. The speed and turn speed are the
// entity sync packet's positionRel and rotationRel.
struct tVehicleSyncPacket
{
	CVector3D rotationFront;
	CVector3D rotationUp;
	CVector3D rotationRight;
	CQuaternion quatRot;
	float rpm;
	float accel;
	float brake;
	float handBrake;
	float clutch;
	float wheelAngle;
};

#define VEHICLESTATE_SOUND (1 << 0)
#define VEHICLESTATE_ENGINE (1 << 1)
#define VEHICLESTATE_HORN (1 << 2)
#define VEHICLESTATE_SIREN (1 << 3)
#define VEHICLESTATE_LIGHTS (1 << 4)
#define VEHICLESTATE_ROOF (1 << 5) // Up (or the model has none)

// Vehicle state that only changes now and then, sent reliably and only when it changes (MAFIAPACKET_VEHICLE_STATE).
// Streaming in gets it from tVehicleCreatePacket.
struct tVehicleStatePacket
{
	float health;
	float engineHealth;
	float fuel;
	float speedLimit;
	int8_t gear;
	uint8_t flags; // VEHICLESTATE_*
};

#define VEHICLEMAFIA2_BEACON (1 << 0)
#define VEHICLEMAFIA2_TAXILIGHT (1 << 1)
#define VEHICLEMAFIA2_HOOD (1 << 2) // Open
#define VEHICLEMAFIA2_TRUNK (1 << 3) // Open
#define VEHICLEMAFIA2_WIPERS (1 << 4)
#define VEHICLEMAFIA2_INDICATORLEFT (1 << 5)
#define VEHICLEMAFIA2_INDICATORRIGHT (1 << 6)
#define VEHICLEMAFIA2_COLOURS (1 << 7) // The colours were set (otherwise the game picks)
#define VEHICLEMAFIA2_TUNING (1 << 8) // The tuning table was set

/*
	Mafia 2 vehicle looks and lights that Mafia 1 has no use for: after the damage in a vehicle's create packet (only on a
	"mafia:two" server, so a Mafia 1 server's packets are unchanged) and in MAFIAPACKET_VEHICLE_MAFIA2.
*/
struct tVehicleMafia2Packet
{
	uint32_t primaryColour; // 0xRRGGBB
	uint32_t secondaryColour;
	char plate[8]; // Empty for the game's own
	float dirt; // 0-1
	int32_t tuning;
	uint16_t flags; // VEHICLEMAFIA2_*
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