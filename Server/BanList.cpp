
#include "pch.h"
#include "BanList.h"

using namespace Galactic3D;

bool CBanList::Load(Stream* pStream)
{
	CTextReader TextReader(pStream);

	size_t Length;
	GChar szBuffer[1024];
	while (TextReader.ReadLine(szBuffer, ARRAY_COUNT(szBuffer), &Length))
	{
		BanIP(szBuffer, 0);
	}

	return true;
}

void CBanList::Clear()
{
	m_Entries.clear();
}

void CBanList::BanIP(const GChar* pszIP, uint32_t uiMilliseconds)
{
	{
		auto it = m_Entries.find(pszIP);
		if (it != m_Entries.end())
		{
			m_Entries.erase(it);
		}
	}
	uint32_t uiExpiry = 0;
	if (uiMilliseconds != 0)
		uiExpiry = OS::GetTicks() + uiMilliseconds;
	m_Entries.emplace(pszIP, tEntry {pszIP, uiExpiry});
}

bool CBanList::Remove(const GChar* pszIP)
{
	auto it = m_Entries.find(pszIP);
	if (it != m_Entries.end())
	{
		m_Entries.erase(it);

		return true;
	}

	return false;
}

bool CBanList::IsBanned(const GChar* pszIP)
{
	auto it = m_Entries.find(pszIP);
	if (it != m_Entries.end())
	{
		if ((*it).second.m_uiExpiry == 0 || OS::GetTicks() < (*it).second.m_uiExpiry)
			return true;
		m_Entries.erase(it);
		return false;
	}
	return false;
}
