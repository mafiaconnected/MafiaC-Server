#pragma once

class CMafiaServerManager;
class CMafiaClient;

class CPeer2PeerSystem
{
public:
	CPeer2PeerSystem(CMafiaServerManager* pManager);

	CMafiaServerManager* m_pManager;

	void ProcessPacket(const tPeerInfo& Peer, unsigned int PacketID, Galactic3D::Stream* pStream);
};
