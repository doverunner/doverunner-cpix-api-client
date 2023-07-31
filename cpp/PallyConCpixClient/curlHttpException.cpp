#include "curlHttpException.h"
#include "util.h"

namespace pallycon {

	CurlHttpError::CurlHttpError(const std::string& msg, int requestStatus, const std::string& reason)
	{
		_httpStatusCode = requestStatus;
		this->_reason = reason;
		this->_msg = msg;

		_what = formatMessage(
			"\n"
			"*** CurlHttpError Exception ***\n"
			"ErrorMessage:[%s]\n"
			"RequestStatus:[%d]\n"
			"RequestReason:[%s]\n"
			, msg.c_str(), requestStatus, reason.c_str());
	}
}