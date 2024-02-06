#pragma once

#include <memory>
#include "CpixClient.h"
#include "curlHttpRequester.h"

namespace pallycon {
	class CpixRequester
	{
	private:

		std::string _serverURL;
		std::string _requestData;
		std::shared_ptr<CurlHttpRequester> _requester;

	public:
		CpixRequester(const std::string& serverURL, std::shared_ptr<CurlHttpRequester> httpRequester)
		{
			_serverURL = serverURL;
			_requester = httpRequester;
		}

		void SetRequestData(const std::string& requestData) {
			_requestData = requestData;
		}

		std::string Request();
	};
}