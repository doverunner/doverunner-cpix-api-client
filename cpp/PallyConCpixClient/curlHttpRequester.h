#pragma once

#include <string>
#include <map>
#include <windows.h>

#include "libCurlHttp.h"

namespace pallycon {
	class CurlHttpRequester
	{
	private:
		enum RequestMethod
		{
			GET,
			POST
		};

		bool _isUTF8;
		LPBYTE _data;
		UrlInfomation _urlInfo;
		std::string _urlParam;

		int _currentCount;
		int _httpStatus;
		std::string _httpReason;
		std::string _location;

		std::string _postRequestData;
		RequestMethod _requestMethod;
		typedef std::map<std::string, std::string> HttpHeaderMap;
		HttpHeaderMap _headerMap;
		std::string _httpResponseStatusLine;
		std::string _httpResponseHeader;

	private:
		static void OnReadHeader(const void* r, void* userdata);
		static void OnReadData(const void* r, void* userdata, const unsigned char* data, int n);
		static void OnReadComplete(const void* r, void* userdata);
		bool isUTF8SignitureIncluded() const;

	public:
		CurlHttpRequester();
		~CurlHttpRequester();

		bool sendRequest();
		void setURL(const std::string& url, const std::string& urlParam);
		std::string getHostURL() const {
			return _urlInfo.getDomain();
		}
		std::string getSubURL() const {
			return _urlInfo.getUri();
		}

		int getPort() const {
			return _urlInfo.getPort();
		}

		int getRequestStatus() const {
			return _httpStatus;
		}

		int getDataLength() const;
		const LPBYTE getData() const;
		void reset();

		std::string getReason() const {
			return _httpReason;
		}

		std::string getFullURL() const;
		std::string getLocation() const;
		void setRequestMethod(RequestMethod method);
		RequestMethod getRequestMethod() const;
		void setPostRequestData(const char* data, UINT len);
		void addHttpHeader(const char* headerName, const char* headerValue);
		std::string getResponseStatusLine() const;
		std::string getResponseHeader() const;
	};
}