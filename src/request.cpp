#include "request.hpp"

namespace http {

request::request(int clientfd, ::http::method method, const ::http::url &url)
	: method(method), url(url), _response(clientfd) {}

response &request::response() { return _response; }

} // namespace http