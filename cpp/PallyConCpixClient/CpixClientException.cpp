#include "CpixClientException.h"

namespace pallycon {
	CpixClientException::CpixClientException(const std::string& msg) : message(msg) {}
	CpixClientException::~CpixClientException() throw() {}
	const char* CpixClientException::what() { return (this->message).c_str(); }
}