#pragma once

#include <Engine/Config.h>

class CServerConfiguration
{
public:
	CServerConfiguration();
	~CServerConfiguration();

	CElementChunk* m_pConfig;

	bool GetBoolValue(const GChar* pszName, bool bDefault) const;
	const GChar* GetStringValue(const GChar* pszName, const GChar* pszDefault) const;
	int32_t GetInt32Value(const GChar* pszName, int32_t iDefault) const;
	float GetFloatValue(const GChar* pszName, float fDefault) const;

	bool Read(Stream* pStream);
};