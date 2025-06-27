#pragma once

class CMafiaServerManager;
class CMafiaClient;

class CPeer2PeerSystem
{
public:
	CPeer2PeerSystem(CMafiaServerManager* pManager);

	CMafiaServerManager* m_pManager;

	void ProcessPacket(Peer_t Peer, unsigned int PacketID, Galactic3D::Stream* pStream);
};
