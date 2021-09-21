#pragma once

#include <unordered_map>

class CBanList
{
public:
	struct tEntry
	{
		GString m_IP;
		uint32_t m_uiExpiry;
	};

	bool Load(Stream* pStream);

	void Clear();
	void BanIP(const GChar* pszIP, uint32_t uiMilliseconds);
	bool Remove(const GChar* pszIP);
	bool IsBanned(const GChar* pszIP);

	std::unordered_map<GString, tEntry> m_Entries;
};
