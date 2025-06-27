#pragma once

#include <Scripting/Resource.h>
#include <Multiplayer/Multiplayer.h>

class CBaseServer;

class CServerResourceMgr : public CNetGameResourceMgr
{
public:
	CServerResourceMgr(Galactic3D::Context* pContext, CBaseServer* pServer);

	CBaseServer* m_pServer;
	//CResource* EnsureLoadedResource(const GChar* pszName, const GChar* pszPath);
	//void UpdateResources(const GPlatformChar* pszFolder);
	void UpdateAResource(CNetServerCompatibilityShim* pNetGame, CResource* pResource);
	void UpdateAllResource(CNetServerCompatibilityShim* pNetGame, const Peer_t Peer);
	virtual void RefreshResourceState(CResource* pResource) override;
	virtual void RemoveThingsAssociatedWithResource(CResource* pResource) override;
};
