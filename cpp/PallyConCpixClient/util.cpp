#include <sstream>
#include <stdarg.h>
#include "util.h"

namespace pallycon {
	const char hexMap[] = { '0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	std::string bytesToHex(unsigned char* data, int len)
	{
		std::string s(len * 2, ' ');
		for (int i = 0; i < len; ++i) {
			s[2 * i] = hexMap[(data[i] & 0xF0) >> 4];
			s[2 * i + 1] = hexMap[data[i] & 0x0F];
		}
		return s;
	}

	void hexToBytes(std::string strHex, unsigned char*& array, int& size)
	{
		int length = strHex.length();

		if (length % 2 == 1)
		{
			strHex = "0" + strHex;
			length++;
		}

		array = new unsigned char[length / 2];
		size = length / 2;

		std::stringstream sstr(strHex);
		for (int i = 0; i < size; i++)
		{
			char ch1, ch2;
			sstr >> ch1 >> ch2;
			int dig1 = 0, dig2 = 0;
			if (isdigit(ch1)) dig1 = ch1 - '0';
			else if (ch1 >= 'A' && ch1 <= 'F') dig1 = ch1 - 'A' + 10;
			else if (ch1 >= 'a' && ch1 <= 'f') dig1 = ch1 - 'a' + 10;
			if (isdigit(ch2)) dig2 = ch2 - '0';
			else if (ch2 >= 'A' && ch2 <= 'F') dig2 = ch2 - 'A' + 10;
			else if (ch2 >= 'a' && ch2 <= 'f') dig2 = ch2 - 'a' + 10;
			array[i] = dig1 * 16 + dig2;
		}
	}

	std::string formatMessage(const char* format, ...)
	{
		const int BUFF_SIZE = 10240;
		char* buf = new char[BUFF_SIZE];
		va_list ap;
		va_start(ap, format);
		int n = vsnprintf(buf, BUFF_SIZE, format, ap);
		va_end(ap);
		if (n < 0 || n > BUFF_SIZE)
		{
			std::cerr << "formatMessage Failed" << std::endl;
			delete[] buf;
			return "";
		}

		std::string result = buf;
		delete[] buf;
		return result;
	}

	UrlInfomation::UrlInfomation(const char* url)
	{
		setURL(url);
	}

	void UrlInfomation::setURL(const char* url)
	{
		parseUrl(url);
	}



	void UrlInfomation::parseUrl(const char* url)
	{
		std::string strURL(url);

		std::string::size_type pos;
		pos = strURL.find("//");
		if (pos == std::string::npos)
		{
			// Invalid URL
			throw std::runtime_error(formatMessage("parseUrl(%s) : Invalid URL #1", url).c_str());
		}

		m_protocol = strURL.substr(0, pos + 2);

		pos = m_protocol.length();
		strURL = strURL.substr(pos, strURL.length() - pos);

		pos = strURL.find('/');
		if (pos == std::string::npos)
		{
			throw std::runtime_error(formatMessage("parseUrl(%s) : Invalid URL #2", url).c_str());
		}

		m_path = strURL.substr(pos, strURL.length() - pos);
		std::string domain = strURL.substr(0, pos);

		// Port Parsing
		pos = domain.find(":");
		if (pos != std::string::npos)
		{
			int nPort = 0;
			++pos;
			std::string port = domain.substr(pos, domain.length() - pos);
			nPort = atoi(port.c_str());
			if (nPort == 0)
			{
				throw std::runtime_error("NcgHttpRequester::setURL() : cannot find port");
			}
			--pos;
			m_port = nPort;
			m_domain = domain.erase(pos, domain.length() - pos);
		}
		else
		{
			if (m_protocol.find("https") != std::string::npos)
			{
				m_port = 443;
			}
			else
			{
				m_port = 80;
			}
			m_domain = domain;
		}
	}

	std::string UrlInfomation::getFullURL() const {
		std::string result;
		if (m_port == 80) {
			result = formatMessage("%s%s%s", m_protocol.c_str(), m_domain.c_str(), m_path.c_str());
		}
		else {
			result = formatMessage("%s%s:%d%s", m_protocol.c_str(), m_domain.c_str(), m_port, m_path.c_str());
		}
		return result;
	}
}