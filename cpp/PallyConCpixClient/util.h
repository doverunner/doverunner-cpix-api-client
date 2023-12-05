#pragma once

#include <string>
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
typedef unsigned int	UINT;
typedef unsigned char	BYTE;
typedef unsigned char*	LPBYTE;
#endif

namespace pallycon {
	std::string bytesToHex(unsigned char* data, int len);
	void hexToBytes(std::string strHex, unsigned char*& array, int& size);
	std::string formatMessage(const char* format, ...);

	class UrlInfomation
	{
	private:
		std::string m_domain;	// ex) www.inka.co.kr
		std::string m_path;		// ex) /subdir/page.html
		std::string m_protocol; // ex) http, ftp, https, etc
		int m_port;


	private:
		void parseUrl(const char* url);

	public:
		UrlInfomation() {
			m_port = 0;
			m_domain = "";
			m_path = "";
		}
		UrlInfomation(const char* url);
		void setURL(const char* url);

		std::string getFullURL() const;

		int getPort() const {
			return m_port;
		}

		std::string getDomain() const {
			return m_domain;
		}

		std::string getUri() const {
			return m_path;
		}

		std::string getProtocol() const {
			return m_protocol;
		}

		std::string getProtocolOnly() const {
			size_t index = m_protocol.find("://");
			if (index != std::string::npos)
			{
				return m_protocol.substr(0, index);
			}
			return m_protocol;
		}
	};
}