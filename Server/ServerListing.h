#pragma once

class CServer;

enum eMasterlistState
{
	MASTERLIST_NONE,
	MASTERLIST_CONNECTING,
	MASTERLIST_CONNECTED,
	MASTERLIST_DISCONNECTED,
	MASTERLIST_DEAD,
};

class CMasterlistAnnouncer : public Galactic3D::WebConnection
{
public:
	CMasterlistAnnouncer(CServer* pServer);

protected:
	CServer* m_pServer;
	float m_fRetryTime;
	eMasterlistState m_OldState;
	eMasterlistState m_State;

	void ChangeState(eMasterlistState State);

public:
	void Pulse(float fDeltaTime);
	virtual void EventHandler(struct mg_connection* nc, int ev, void* ev_data) override;
	virtual void ProcessPacket(mg_connection* nc, unsigned int PacketID, Stream* pStream) override;

	void PlayerAdd(CNetMachine* pNetMachine);
	void PlayerRemove(CNetMachine* pNetMachine);
	void SetGameMode(const GChar* pszGameMode, size_t Length);
	void SetName(const GChar* pszName, size_t Length);

	virtual void OnPlayerConnect(mg_connection *nc) override;
	virtual void OnPlayerDisconnect(mg_connection *nc) override;
};