#pragma once

class CServer;

class CRConServer : public WebConnection
{
public:
	CRConServer(CServer* pServer);

	CServer* m_pServer;
	CSecurePassword m_Password;

	virtual void ProcessPacket(mg_connection* nc, unsigned int PacketID, Stream* pStream) override;
	virtual void EventHandler(struct mg_connection* nc, int ev, void* ev_data) override;

	void Log(mg_connection* nc, eLogType LogType, const GChar* psz);
	void Auth(mg_connection* nc, bool b);
};