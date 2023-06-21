#pragma once

#include "method.hpp"
#include "url.hpp"
#include "response.hpp"

namespace http {

struct request {
	request(int clientfd, ::http::method method, const ::http::url &url);

	const ::http::method method;
	const ::http::url &url;

	::http::response &response();

  private:
	::http::response _response;
}; // request

} // namespace http
