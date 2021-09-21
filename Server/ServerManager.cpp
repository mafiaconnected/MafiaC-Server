
#include "pch.h"
#include "ServerManager.h"
#include "Server.h"

using namespace Galactic3D;

CServerManager::CServerManager(Context* pContext, CServer* pServer) :
	CManager(pContext),
	CNetObjectMgr(&pServer->m_ResourceMgr, true)
{
	m_pServer = pServer;
	m_pNetServer = pServer;

	pServer->m_ResourceMgr.m_pManager = this;

	RegisterFunctions(m_pServer->m_ResourceMgr.m_pScripting);
}

static bool FunctionTriggerNetworkEvent(IScriptState* pState, int argc, void* pUser)
{
	CServerManager* pServerManager = (CServerManager*)pUser;
	size_t NameLength;
	const GChar* pszName = pState->CheckString(0, &NameLength);
	if (!pszName)
		return false;
	unsigned int nNameHash = Galactic3D::CRC32Hash::GetHash(pszName, 0, true);
	CNetMachine* pNetMachine = nullptr;
	if (!pState->CheckClass(pServerManager->m_pNetMachineClass, 1, true, &pNetMachine))
		return false;
	CArguments Args(argc - 2);
	for (int i = 2; i < argc; i++)
		Args.Add(pState->GetArgument(i));

	Packet Packet(PACKET_NETWORKEVENT);
	CBinaryWriter Writer(&Packet);
	Writer.WriteString(pszName, NameLength);

	Args.Write(&Packet);

	if (pNetMachine == nullptr)
		pServerManager->m_pServer->SendEveryonePacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_EVENTS);
	else
		pNetMachine->SendPacket(&Packet, PACKETPRIORITY_DEFAULT, PACKETFLAGS_RELIABLE, PACKETORDERINGCHANNEL_EVENTS);

	return true;
}

void CServerManager::RegisterFunctions(CScripting* pScripting)
{
	pScripting->m_Global.RegisterFunction(_gstr("triggerNetworkEvent"), _gstr("sx*"), FunctionTriggerNetworkEvent, this);
}
