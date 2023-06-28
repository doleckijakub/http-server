#pragma once

#include "method.hpp"
#include "url.hpp"
#include "response.hpp"

namespace http {

struct request {
	request(int clientfd, ::http::method method, const ::http::url &url,
			const std::unordered_map<std::string, std::string> &headers,
			const std::unordered_map<std::string, std::string> &payload);

	const ::http::method method;
	const ::http::url &url;

	::http::response &response();

	const std::string &getHeader(const std::string &name);
	const std::string &getPayloadParameter(const std::string &key);

  private:
	::http::response _response;

	const std::unordered_map<std::string, std::string> &_headers;
	const std::unordered_map<std::string, std::string> &_payload; // POST-only

}; // request

} // namespace http
