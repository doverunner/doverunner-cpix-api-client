#include <windows.h>
#ifndef _RIGHTS_DAEMON_BASE64_HEADER_FILE
#define _RIGHTS_DAEMON_BASE64_HEADER_FILE

namespace pallycon {
	const char* Base64Encode(LPBYTE pbData, int nLength, int* pnResultSize);
	LPBYTE Base64Decode(const char*, int* pnLength);
}

#endif // _RIGHTS_DAEMON_BASE64_HEADER_FILE
