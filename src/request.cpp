#include "request.hpp"

namespace http {

request::request(int clientfd, ::http::method method, const ::http::url &url,
				 const std::unordered_map<std::string, std::string> &headers,
				 const std::unordered_map<std::string, std::string> &payload)
	: method(method), url(url), _response(clientfd), _headers(headers), _payload(payload) {
}

response &request::response() {
	return _response;
}

const std::string &request::getHeader(const std::string &name) {
	const static std::string empty = {};
	auto it = _headers.find(name);
	return (it == _headers.end()) ? empty : it->second;
}

const std::string &request::getPayloadParameter(const std::string &key) {
	const static std::string empty = {};
	auto it = _payload.find(key);
	return (it == _payload.end()) ? empty : it->second;
}

} // namespace http