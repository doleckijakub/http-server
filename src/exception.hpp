#pragma once

#include <string>

namespace http {

struct exception {
	const int code;
	const std::string message;
	exception(int code, std::string message);
}; // exception

} // namespace http
