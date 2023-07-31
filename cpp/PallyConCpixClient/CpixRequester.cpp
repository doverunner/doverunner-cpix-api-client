#include "CpixRequester.h"

namespace pallycon {
	std::string CpixRequester::Request()
	{
		_requester->setURL(_serverURL, _requestData);
		_requester->addHttpHeader("Content-type", "application/xml");

		int retry_cnt = 0;
		while (_requester->sendRequest() == false)
		{
			retry_cnt++;
			if (_requester->getRequestStatus() != HTTP_REQUEST_ERROR
				|| retry_cnt > 3)
				return "";

			std::string retry_msg = "[CpixRequester::request] Request failed. Retry count : " + std::to_string(retry_cnt) + "\n";
			fprintf(stdout, retry_msg.c_str());
		}

		const LPBYTE data = _requester->getData();
		if (data == NULL) {
			throw std::runtime_error("[CpixRequester::request] request data is NULL");
		}

		int copyIndex = 0;
		if (data[0] == 0xef && data[1] == 0xbb && data[2] == 0xbf) {
			copyIndex += 3;
		}

		int dataLen = _requester->getDataLength();
		std::string responseData;
		std::copy(&data[copyIndex], &data[dataLen], std::back_inserter(responseData));

		return responseData;
	}
}