#pragma once
#include <string>
#include <map>
#ifdef WIN32
#define CURL_STATICLIB
//#if WINAPI_FAMILY_PARTITION(WINAPI_FAMILY_PC_APP)
#if defined(WINAPI_FAMILY) && WINAPI_FAMILY == WINAPI_FAMILY_PC_APP
#include <winsock2.h>
#endif
#endif
#include <curl/curl.h>
#include <stdarg.h>

#include "util.h"

#define HTTP_REQUEST_OK				200
#define HTTP_REQUEST_ERROR			0x10001000
#define HTTP_REQUEST_NOT_FOUND		404

namespace pallycon {
	typedef void(*ResponseHeader)(const void* r, void* userdata);
	typedef void(*ResponseData)(const void* r, void* userdata, const unsigned char* data, int numbytes);
	typedef void(*ResponseComplete)(const void* r, void* userdata);
		
	class LibCurlHttp
	{
	public:
		LibCurlHttp();
		~LibCurlHttp();

		void runHttp();
		void setcallbacks(ResponseHeader responseHeader, ResponseData responseData, ResponseComplete responseComplete, void* userData);
		void putRequest(std::string requestType, std::string path, const char* parameter);
		void putHeader(std::string key, std::string value);

		std::string getHeader(std::string key);

		std::string getResponseHeaderString()
		{
			return responseHeaderString;
		}

		std::string getRequestHeaderString()
		{
			return requestHeaderString;
		}

		int getResponseStatusCode()
		{
			return responseHttpStatus;
		}

		std::string getResponseStatusMessage()
		{
			return responseMessage;
		}

		void close();

	public:
		std::string exceptionMessage;

		typedef std::map<std::string, std::string> HttpHeaderMap;

		CURL* curlObj;
		curl_slist* headerChunk;

		bool _isUTF8;
		bool isCallResponseHeader;

		HttpHeaderMap	requestHeaderMap;
		HttpHeaderMap	responseHeaderMap;

		long long	responseDataCount;

		int			responseHttpStatus;
		std::string responseMessage;

		std::string		requestData;
		std::string		requestHeaderString;
		std::string		responseHeaderString;

		ResponseHeader		responseHeader;
		ResponseData		responseData;
		ResponseComplete	responseComplete;
		void* userData;

	private:
		//libcurl callback
		//size_t callbackHeader(void *header, size_t size, size_t count, void *userData);
		//size_t callbackData(void *data, size_t size, size_t count, void *userData);

		std::string connectionUrl;

		bool isCheckReadTimeout;
	};
}