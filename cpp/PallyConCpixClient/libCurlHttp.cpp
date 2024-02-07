#include <string.h>
#include <list>
#include <vector>
#include <algorithm>

#include "libCurlHttp.h"
#include "curlHttpException.h"

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P)          (P)
#endif


#define BUFFER_SIZE 4096

namespace pallycon {
	std::string trimString(const std::string& s)
	{
		if (s.length() == 0)
			return s;

		std::string whiteChar = " \a\b\f\n\r\t\v";
		std::size_t beg = s.find_first_not_of(whiteChar);
		std::size_t end = s.find_last_not_of(whiteChar);
		if (beg == std::string::npos) // No non-spaces
			return s;

		return std::string(s, beg, end - beg + 1);
	}

	char easytolower(char in) {
		if (in <= 'Z' && in >= 'A')
			return in - ('Z' - 'z');
		return in;
	}

	int split(const std::string& pattern, const std::string& subject, std::vector<std::string>* Dest)
	{
		std::string::size_type pattern_length = pattern.length();
		std::string::size_type beginpos = 0;
		std::string::size_type endpos = subject.find(pattern);
		while (endpos != std::string::npos)
		{
			Dest->push_back(subject.substr(beginpos, endpos - beginpos));
			beginpos = endpos + pattern_length;
			endpos = subject.find(pattern, beginpos);
		}

		Dest->push_back(trimString(subject.substr(beginpos).c_str()));
		return 0;
	}

	size_t callbackHeader(void* header, size_t size, size_t count, void* userData)
	{
		LibCurlHttp* libCurl = (LibCurlHttp*)userData;
		size_t headerSize = size * count;
		std::string headerString = (const char*)header;
		if (libCurl->responseHttpStatus == 0 && headerString.find("HTTP") != std::string::npos)
		{
			curl_easy_getinfo(libCurl->curlObj, CURLINFO_HTTP_CODE, &libCurl->responseHttpStatus);
			if (libCurl->responseHttpStatus >= 400)
			{
				libCurl->exceptionMessage = formatMessage("LibCurlHttp::callbackHeader() connection error statusCode = %d", libCurl->responseHttpStatus);
				return 0;
			}
			return headerSize;
		}

		libCurl->responseHeaderString.append(headerString);

		std::vector<std::string> result;
		split(":", headerString.c_str(), &result);
		if (result.size() == 2)
		{
			std::string key = result.at(0);
			std::transform(key.begin(), key.end(), key.begin(), easytolower);

			std::string value = result.at(1);
			libCurl->responseHeaderMap[key] = value;
		}

		return headerSize;
	}

	size_t callbackData(void* data, size_t size, size_t count, void* userData)
	{
		LibCurlHttp* libCurl = (LibCurlHttp*)userData;
		size_t dataSize = size * count;

		try {
			if (libCurl->isCallResponseHeader == false)
			{
				libCurl->isCallResponseHeader = true;
				if (libCurl->responseHeader)
				{
					(libCurl->responseHeader)(libCurl, libCurl->userData);
				}
			}

			if (libCurl->responseData)
			{
				(libCurl->responseData)(libCurl, libCurl->userData,
					(const unsigned char*)data,
					dataSize);
			}
			libCurl->responseDataCount += dataSize;

			return dataSize;
		}
		catch (std::exception& e) {
			libCurl->exceptionMessage = e.what();
			return 0;
		}
	}

	LibCurlHttp::LibCurlHttp()
	{
		responseHeaderMap.clear();
		isCheckReadTimeout = false;
		curlObj = NULL;
		headerChunk = NULL;
		responseHttpStatus = 0;
		responseDataCount = 0;
		isCallResponseHeader = false;

		curl_global_init(CURL_GLOBAL_ALL);
		curlObj = curl_easy_init();
	}

	LibCurlHttp::~LibCurlHttp()
	{
		responseHeaderMap.clear();
		responseDataCount = 0;
		close();
	}

	void LibCurlHttp::close()
	{
		if (headerChunk != NULL)
		{
			curl_slist_free_all(headerChunk);
		}

		if (curlObj != NULL)
		{
			curl_easy_cleanup(curlObj);
		}

		curl_global_cleanup();
	}

	void LibCurlHttp::setcallbacks(ResponseHeader responseHeader, ResponseData responseData, ResponseComplete responseComplete, void* userData)
	{
		this->responseHeader = responseHeader;
		this->responseData = responseData;
		this->responseComplete = responseComplete;
		this->userData = userData;
	}

	void LibCurlHttp::runHttp()
	{
		if (curlObj == NULL)
		{
			throw CurlHttpError("LibCurlHttp::runHttp()", HTTP_REQUEST_ERROR, "Unable to initialize cURL interface");
		}

		CURLcode res;
		if (headerChunk != NULL)
		{
			res = curl_easy_setopt(curlObj, CURLOPT_HTTPHEADER, headerChunk);
			curl_easy_setopt(curlObj, CURLOPT_VERBOSE, 0L);
		}

		res = curl_easy_perform(curlObj);
		/* Check for errors */
		if (res != CURLE_OK) {
			if (res == CURLE_WRITE_ERROR && exceptionMessage.empty() == false) {
				throw CurlHttpError("LibCurlHttp::runHttp()", HTTP_REQUEST_ERROR, exceptionMessage.c_str());
			}
			std::string errorMessage = curl_easy_strerror(res);
			throw CurlHttpError("LibCurlHttp::runHttp()", HTTP_REQUEST_ERROR, errorMessage.c_str());
		}

		if (responseComplete)
		{
			(responseComplete)(this, userData);
		}
	}

	void LibCurlHttp::putRequest(std::string requestType, std::string path, const char* parameter)
	{
		UNREFERENCED_PARAMETER(requestType);
		if (curlObj == NULL)
		{
			throw CurlHttpError("LibCurlHttp::putRequest()", HTTP_REQUEST_ERROR, "Unable to initialize cURL interface");
		}
		connectionUrl = path;

		curl_easy_setopt(curlObj, CURLOPT_CUSTOMREQUEST, requestType.c_str());
		curl_easy_setopt(curlObj, CURLOPT_URL, connectionUrl.c_str());

		if (parameter != NULL && strlen(parameter) > 0)
		{
			curl_easy_setopt(curlObj, CURLOPT_POSTFIELDS, parameter);
			curl_easy_setopt(curlObj, CURLOPT_POSTFIELDSIZE, strlen(parameter));
		}

		curl_easy_setopt(curlObj, CURLOPT_BUFFERSIZE, BUFFER_SIZE);
		curl_easy_setopt(curlObj, CURLOPT_NOPROGRESS, true);

		curl_easy_setopt(curlObj, CURLOPT_WRITEFUNCTION, callbackData);
		curl_easy_setopt(curlObj, CURLOPT_WRITEDATA, this);

		curl_easy_setopt(curlObj, CURLOPT_HEADERFUNCTION, callbackHeader);
		curl_easy_setopt(curlObj, CURLOPT_WRITEHEADER, this);

		curl_easy_setopt(curlObj, CURLOPT_COOKIESESSION, true);

		curl_easy_setopt(curlObj, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curlObj, CURLOPT_SSL_VERIFYHOST, 0L);

		curl_easy_setopt(curlObj, CURLOPT_NOSIGNAL, 1L);

		// It is needed for redirect
		curl_easy_setopt(curlObj, CURLOPT_POSTREDIR, CURL_REDIR_POST_301);
		curl_easy_setopt(curlObj, CURLOPT_FOLLOWLOCATION, 1L);

		// PD ��� �� Seek �� timeout�� �߻�(iOS)�Ͽ� �ش� �ɼ��� �����Ѵ�.
		//curl_easy_setopt(curlObj, CURLOPT_TIMEOUT, 10);
	}

	void LibCurlHttp::putHeader(std::string key, std::string value)
	{
		std::string header = formatMessage("%s: %s", key.c_str(), value.c_str());
		headerChunk = curl_slist_append(headerChunk, header.c_str());
		requestHeaderString.append(header);
		requestHeaderString.append("\r\n");
	}

	std::string LibCurlHttp::getHeader(std::string key)
	{
		if (responseHeaderMap.find(key) == responseHeaderMap.end())
		{
			return "";
		}

		return responseHeaderMap[key];
	}
}