#pragma once

#include <stdint.h>
#include <stddef.h>

// Standard (RFC 4648) base64, for handing binary data to scripts as a string they can store anywhere. Templated on
// the character type so it works with GChar whichever width it is.
namespace Base64
{
	inline size_t GetEncodedLength(size_t Size)
	{
		return ((Size + 2) / 3) * 4;
	}

	// Writes GetEncodedLength(Size) characters to pOut, no terminator
	template<typename TChar>
	inline void Encode(const uint8_t* pData, size_t Size, TChar* pOut)
	{
		static const char Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		size_t o = 0;
		for (size_t i = 0; i < Size; i += 3)
		{
			uint32_t Value = (uint32_t)pData[i] << 16;
			if (i + 1 < Size)
				Value |= (uint32_t)pData[i + 1] << 8;
			if (i + 2 < Size)
				Value |= pData[i + 2];

			pOut[o++] = (TChar)Alphabet[(Value >> 18) & 0x3F];
			pOut[o++] = (TChar)Alphabet[(Value >> 12) & 0x3F];
			pOut[o++] = (i + 1 < Size) ? (TChar)Alphabet[(Value >> 6) & 0x3F] : (TChar)'=';
			pOut[o++] = (i + 2 < Size) ? (TChar)Alphabet[Value & 0x3F] : (TChar)'=';
		}
	}

	// Upper bound of the decoded size of Length characters
	inline size_t GetMaxDecodedLength(size_t Length)
	{
		return (Length / 4) * 3;
	}

	// pOut needs GetMaxDecodedLength(Length) bytes. Size gets the real decoded size. False if it isn't valid base64.
	template<typename TChar>
	inline bool Decode(const TChar* psz, size_t Length, uint8_t* pOut, size_t& Size)
	{
		Size = 0;

		if ((Length % 4) != 0)
			return false;

		for (size_t i = 0; i < Length; i += 4)
		{
			uint32_t Value = 0;
			size_t Padding = 0;

			for (size_t j = 0; j < 4; j++)
			{
				TChar c = psz[i + j];
				uint32_t Bits;

				if (c >= 'A' && c <= 'Z')
					Bits = (uint32_t)(c - 'A');
				else if (c >= 'a' && c <= 'z')
					Bits = (uint32_t)(c - 'a') + 26;
				else if (c >= '0' && c <= '9')
					Bits = (uint32_t)(c - '0') + 52;
				else if (c == '+')
					Bits = 62;
				else if (c == '/')
					Bits = 63;
				else if (c == '=' && j >= 2 && i + 4 == Length)
				{
					Bits = 0;
					Padding++;
				}
				else
					return false;

				// Nothing but padding may follow padding
				if (Padding != 0 && c != '=')
					return false;

				Value = (Value << 6) | Bits;
			}

			pOut[Size++] = (uint8_t)(Value >> 16);
			if (Padding < 2)
				pOut[Size++] = (uint8_t)(Value >> 8);
			if (Padding < 1)
				pOut[Size++] = (uint8_t)Value;
		}

		return true;
	}
};
