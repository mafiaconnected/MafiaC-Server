
#include "pch.h"
#include "Server.h"
#include "UnleashedGameProtocolHandler.h"

using namespace Galactic3D;

CUnleashedGameProtocolHandler::CUnleashedGameProtocolHandler(CServer* pServer) :
	m_pServer(pServer)
{
	m_pResponse = new CUGPResponse;
}

CUnleashedGameProtocolHandler::~CUnleashedGameProtocolHandler()
{
	delete m_pResponse;
}

void CUnleashedGameProtocolHandler::UpdateResponse()
{
	CLockable::CLock Lock(&m_Lock, true);

	m_pResponse->m_ucGameID = (uint8_t)m_pServer->m_GameId;

	_gstrcpy_s(m_pResponse->m_ServerInfo.m_szGameName, ARRAY_COUNT(m_pResponse->m_ServerInfo.m_szGameName), _gstr("Mafia Connected"));
	m_pResponse->m_ServerInfo.m_bPassworded = m_pServer->m_Password.HasPassword();
	_gstrcpy_s(m_pResponse->m_ServerInfo.m_szVersion, ARRAY_COUNT(m_pResponse->m_ServerInfo.m_szVersion), _gstr(""));
	_gstrcpy_s(m_pResponse->m_ServerInfo.m_szName, ARRAY_COUNT(m_pResponse->m_ServerInfo.m_szName), m_pServer->GetServerName());
	_gstrcpy_s(m_pResponse->m_ServerInfo.m_szGameMode, ARRAY_COUNT(m_pResponse->m_ServerInfo.m_szGameMode), m_pServer->GetGameMode());
	_gstrcpy_s(m_pResponse->m_ServerInfo.m_szMap, ARRAY_COUNT(m_pResponse->m_ServerInfo.m_szMap), m_pServer->m_szLevel);
	m_pResponse->m_ServerInfo.m_PlayerCount = (uint8_t)m_pServer->m_CurrentClients;
	m_pResponse->m_ServerInfo.m_MaxPlayers = (uint8_t)m_pServer->m_MaxClients;

	size_t PlayerCount = 0;
	for (size_t i = 0; i < ARRAY_COUNT(m_pServer->m_NetMachines.m_rgpMachines); i++)
	{
		auto pNetMachine = m_pServer->m_NetMachines.GetMachine(i);
		if (pNetMachine != nullptr && !pNetMachine->m_bConsole)
		{
			auto& Player = m_pResponse->m_rgPlayers[PlayerCount];

			Player.m_ucID = (uint8_t)pNetMachine->m_nIndex;
			_gstrcpy_s(Player.m_szName, ARRAY_COUNT(Player.m_szName), pNetMachine->GetName());

			PlayerCount++;

			if (PlayerCount >= ARRAY_COUNT(m_pResponse->m_rgPlayers))
				break;
		}
	}
	m_pResponse->m_PlayerCount = PlayerCount;

	size_t RulesCount = 0;
	for (const auto& ServerRule : m_pServer->m_Rules)
	{
		auto& Rule = m_pResponse->m_rgRules[RulesCount];

		_gstrcpy_s(Rule.m_szKey, ARRAY_COUNT(Rule.m_szKey), ServerRule.m_Key.c_str());
		_gstrcpy_s(Rule.m_szValue, ARRAY_COUNT(Rule.m_szValue), ServerRule.m_Value.c_str());

		RulesCount++;

		if (RulesCount >= ARRAY_COUNT(m_pResponse->m_rgRules))
			break;
	}
	m_pResponse->m_RulesCount = RulesCount;
}

bool CUnleashedGameProtocolHandler::ReceiveDatagram(CNetSocket* pNetSocket)
{
	size_t Length;
	void* pData = pNetSocket->GetData(Length);
	if (Length >= 2 && memcmp(pData, "\xFF\xFF", 2) == 0)
	{
		CMemoryStream Stream(pData, Length, true, false);

		CUGPRequest Request;
		if (Request.Read(&Stream))
		{
			Packet Packet;
			{
				CLockable::CLock Lock(&m_Lock, true);
				if (!m_pResponse->Write(&Packet, Request))
					return true;
			}
			pNetSocket->Send(&Packet);
		}
		return true;
	}
	return false;
}
