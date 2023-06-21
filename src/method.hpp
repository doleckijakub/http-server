#pragma once

namespace http {

enum class method {
	UNKNOWN,
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	CONNECT,
	OPTIONS,
	TRACE,
	PATCH,
}; // method

} // namespace http
