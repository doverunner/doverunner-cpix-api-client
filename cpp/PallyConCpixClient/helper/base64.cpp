#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "base64.h"

namespace pallycon {
	static char __base64_table[] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '\0'
	};

	static int __reverse_table[] = {
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
		-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
		-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	};

	static char __base64_pad = '=';

	const char* Base64Encode(unsigned char* pbData, int nLength, int* pnResultSize)
	{
		int					i, sizeOfBuffer;
		char* pszResult = NULL;

		sizeOfBuffer = ((nLength + 3 - nLength % 3) * 4 / 3 + 1);
		pszResult = new char[sizeOfBuffer];

		i = 0;
		while (nLength > 2)
		{
			/* keep going until we have less than 24 bits */
			pszResult[i++] = __base64_table[pbData[0] >> 2];
			pszResult[i++] = __base64_table[((pbData[0] & 0x03) << 4) + (pbData[1] >> 4)];
			pszResult[i++] = __base64_table[((pbData[1] & 0x0f) << 2) + (pbData[2] >> 6)];
			pszResult[i++] = __base64_table[pbData[2] & 0x3f];

			pbData += 3;
			nLength -= 3; /* we just handle 3 octets of data */
		}

		/* now deal with the tail end of things */
		if (nLength != 0)
		{
			pszResult[i++] = __base64_table[pbData[0] >> 2];
			if (nLength == 1)
			{
				pszResult[i++] = __base64_table[(pbData[0] & 0x03) << 4];
				pszResult[i++] = __base64_pad;
				pszResult[i++] = __base64_pad;
			}
			else
			{
				pszResult[i++] = __base64_table[((pbData[0] & 0x03) << 4) + (pbData[1] >> 4)];
				pszResult[i++] = __base64_table[(pbData[1] & 0x0f) << 2];
				pszResult[i++] = __base64_pad;
			}
		}

		pszResult[i] = 0;
		*pnResultSize = i;

		return pszResult;
	}


	unsigned char* Base64Decode(const char* pszString, int* pnLength)
	{
		int					result;
		unsigned char* pbResult = NULL;
		int					nStrLength, i, nOutputLength;
		unsigned char		b1, b2, b3, b4;

		nStrLength = (int)strlen(pszString);
		if (nStrLength % 4 != 0)
		{
			result = -1;
			goto finish;
		}

		pbResult = new unsigned char[nStrLength + 1];

		nOutputLength = 0;
		for (i = 0; i < nStrLength; i += 4)
		{
			b1 = (unsigned char)__reverse_table[pszString[i]];
			b2 = (unsigned char)__reverse_table[pszString[i + 1]];
			b3 = (unsigned char)__reverse_table[pszString[i + 2]];
			b4 = (unsigned char)__reverse_table[pszString[i + 3]];

			pbResult[nOutputLength++] = (unsigned char)((b1 << 2) | (b2 >> 4));

			if (pszString[i + 2] == '=')
			{
				pbResult[nOutputLength] = (unsigned char)((b2 & 0x0F) << 4);
			}
			else
			{
				pbResult[nOutputLength++] = (unsigned char)(((b2 & 0x0F) << 4) | (b3 >> 2));
				if (pszString[i + 3] == '=')
				{
					pbResult[nOutputLength] = (unsigned char)((b3 & 0x03) << 6);
				}
				else
				{
					pbResult[nOutputLength++] = (unsigned char)(((b3 & 0x03) << 6) | b4);
				}
			}
		}

		*pnLength = nOutputLength;
		result = 0;

	finish:
		if (result != 0)
		{
			if (pbResult != NULL)
			{
				free(pbResult);
				pbResult = NULL;
			}
		}

		return pbResult;
	}
}