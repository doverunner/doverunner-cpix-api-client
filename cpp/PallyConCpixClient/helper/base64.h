namespace pallycon {
	const char* Base64Encode(unsigned char* pbData, int nLength, int* pnResultSize);
	unsigned char* Base64Decode(const char*, int* pnLength);
}
