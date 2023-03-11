
#include "pch.h"
#include "ServerConfiguration.h"

CServerConfiguration::CServerConfiguration()
{
	m_pConfig = nullptr;
}

CServerConfiguration::~CServerConfiguration()
{
	if (m_pConfig != nullptr)
		delete m_pConfig;
}

bool CServerConfiguration::GetBoolValue(const GChar* pszName, bool bDefault) const
{
	auto pElement = m_pConfig->FirstChildElement(pszName);
	if (pElement != nullptr)
	{
		const GChar* pszText = pElement->GetText();
		if (pszText != nullptr)
			return StringManager::ToBoolean(pszText, bDefault);
	}
	return bDefault;
}

const GChar* CServerConfiguration::GetStringValue(const GChar* pszName, const GChar* pszDefault) const
{
	auto pElement = m_pConfig->FirstChildElement(pszName);
	if (pElement != nullptr)
	{
		const GChar* pszText = pElement->GetText();
		if (pszText != nullptr)
			return pszText;
	}
	return pszDefault;
}

int32_t CServerConfiguration::GetInt32Value(const GChar* pszName, int32_t iDefault) const
{
	auto pElement = m_pConfig->FirstChildElement(pszName);
	if (pElement != nullptr)
	{
		const GChar* pszText = pElement->GetText();
		if (pszText != nullptr)
			return _gstrtol(pszText, nullptr, 10);
	}
	return iDefault;
}

float CServerConfiguration::GetFloatValue(const GChar* pszName, float fDefault) const
{
	auto pElement = m_pConfig->FirstChildElement(pszName);
	if (pElement != nullptr)
	{
		const GChar* pszText = pElement->GetText();
		if (pszText != nullptr)
			return _gstrtof(pszText, nullptr);
	}
	return fDefault;
}

bool CServerConfiguration::Read(Stream* pStream)
{
	CElementChunk* pConfig = CElementChunk::Load(pStream);
	if (pConfig != nullptr)
	{
		m_pConfig = pConfig;
		return true;
	}
	return false;
}
