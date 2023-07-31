#pragma once
#include <string>

namespace pallycon {

	class CurlHttpError : public std::exception
	{
	private:
		std::string _what;
		int _httpStatusCode;
		std::string _reason;
		std::string _msg;

	public:
		CurlHttpError(const std::string& msg, int requestStatus, const std::string& reason);

		virtual ~CurlHttpError() {

		}

		virtual const char* what() const throw()
		{
			return _what.c_str();
		}

		int getHttpStatusCode() {
			return _httpStatusCode;
		}
	};

}

