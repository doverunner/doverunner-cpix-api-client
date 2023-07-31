#include <memory>
#include "curlHttpRequester.h"
#include "curlHttpException.h"
#include "util.h"

#define MAX_REQUEST_DATA_SIZE		1024*1024

namespace pallycon {
	CurlHttpRequester::CurlHttpRequester()
	{
		_data = NULL;
		_requestMethod = POST;
		reset();
	}

	CurlHttpRequester::~CurlHttpRequester()
	{
		delete[] _data;
	}

	void CurlHttpRequester::reset()
	{
		if (_data != NULL)
		{
			delete[] _data;
			_data = NULL;
		}
		_currentCount = 0;
		_isUTF8 = false;
	}

	void CurlHttpRequester::OnReadHeader(const void* r, void* userdata)
	{
		CurlHttpRequester* userThis = (CurlHttpRequester*)userdata;
		LibCurlHttp* object = (LibCurlHttp*)r;
		userThis->_httpStatus = object->getResponseStatusCode();
		userThis->_httpReason = object->getResponseStatusMessage();
		userThis->_httpResponseHeader = object->getResponseHeaderString();

		std::string charset = object->getHeader("content-type");
		if (charset.find("utf-8") != std::string::npos)
		{
			userThis->_isUTF8 = true;
		}

		if (object->getHeader("content-length").empty() == false)
		{
			int contentLength = atoi(object->getHeader("content-length").c_str());
			userThis->_data = new BYTE[contentLength];
		}
		else
		{
			userThis->_data = new BYTE[MAX_REQUEST_DATA_SIZE];
		}
	}

	void CurlHttpRequester::OnReadData(const void* r, void* userdata, const unsigned char* data, int n)
	{
		CurlHttpRequester* userThis = (CurlHttpRequester*)userdata;
		memcpy(&userThis->_data[userThis->_currentCount], data, n);
		userThis->_currentCount += n;
	}

	void CurlHttpRequester::OnReadComplete(const void* r, void* userdata)
	{
		UNREFERENCED_PARAMETER(r);
		UNREFERENCED_PARAMETER(userdata);
	}

	int CurlHttpRequester::getDataLength() const
	{
		if (isUTF8SignitureIncluded())
		{
			return _currentCount - 3;
		}
		return _currentCount;
	}


	const LPBYTE CurlHttpRequester::getData() const
	{
		if (isUTF8SignitureIncluded())
		{
			return (_data + 3);

		}
		return _data;
	}

	bool CurlHttpRequester::sendRequest()
	{
		try
		{
			reset();
			std::shared_ptr<LibCurlHttp> httpObject(new LibCurlHttp());
			httpObject->setcallbacks(OnReadHeader, OnReadData, NULL, this);

			if ((_requestMethod == POST &&
				_postRequestData.length() > 0) || _urlParam.length() > 0)
			{
				httpObject->putRequest("POST", _urlInfo.getFullURL(), _urlParam.c_str());
				HttpHeaderMap::iterator iter;
				for (iter = _headerMap.begin(); iter != _headerMap.end(); iter++)
				{
					httpObject->putHeader(iter->first, iter->second);
				}

				if (_postRequestData.length() != 0)
				{
					httpObject->putHeader("Content-Length", formatMessage("%d", (int)_postRequestData.length()));
				}
				else
				{
					httpObject->putHeader("Connection", "close");
					httpObject->putHeader("Content-type", "application/x-www-form-urlencoded");
					httpObject->putHeader("Accept", "text/plain");

					if (_urlParam.empty() == false)
					{
						httpObject->putHeader("Content-Length", formatMessage("%d", (int)_urlParam.length()));
					}
				}
			}
			else // GET method
			{
				httpObject->putRequest("GET", _urlInfo.getFullURL(), _urlParam.c_str());
				HttpHeaderMap::iterator iter;
				for (iter = _headerMap.begin(); iter != _headerMap.end(); iter++)
				{
					httpObject->putHeader(iter->first, iter->second);
				}

				if (_urlParam.empty() == false)
				{
					httpObject->putHeader("Content-Length", formatMessage("%d", (int)_urlParam.length()));
				}
			}

			httpObject->runHttp();
		}
		catch (CurlHttpError& e)
		{
			_httpReason = e.what();
			_httpStatus = e.getHttpStatusCode();
			return false;
		}
		catch (std::exception& e)
		{
			_httpReason = e.what();
			_httpStatus = HTTP_REQUEST_ERROR;
			return false;
		}

		if (getRequestStatus() >= 400)
		{
			return false;
		}

		return true;
	}

	bool CurlHttpRequester::isUTF8SignitureIncluded() const
	{
		BYTE utf8Signature[] = { 0xEF, 0xBB, 0xBF };
		if (_isUTF8 && _currentCount >= 3 && memcmp(_data, utf8Signature, 3) == 0)
		{
			return true;
		}

		return false;
	}

	void CurlHttpRequester::setURL(const std::string& url, const std::string& urlParam)
	{
		_urlInfo.setURL(url.c_str());
		_urlParam = urlParam;
	}


	std::string CurlHttpRequester::getFullURL() const
	{
		if (_urlParam.empty() == false)
		{
			return _urlInfo.getFullURL() + "?" + _urlParam;
		}
		else
		{
			return _urlInfo.getFullURL();
		}
	}

	std::string CurlHttpRequester::getLocation() const
	{
		return _location;
	}

	CurlHttpRequester::RequestMethod CurlHttpRequester::getRequestMethod() const
	{
		return _requestMethod;
	}

	void CurlHttpRequester::setRequestMethod(RequestMethod method)
	{
		_requestMethod = method;
	}

	void CurlHttpRequester::setPostRequestData(const char* data, UINT len)
	{
		_postRequestData.assign(data, len);
	}

	void CurlHttpRequester::addHttpHeader(const char* headerName, const char* headerValue)
	{
		_headerMap[headerName] = headerValue;
	}

	std::string CurlHttpRequester::getResponseStatusLine() const
	{
		return _httpResponseStatusLine;
	}

	std::string CurlHttpRequester::getResponseHeader() const
	{
		return _httpResponseHeader;
	}
}