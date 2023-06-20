#include "http.hpp"

#include <cerrno>
#include <cstring>
#include <string>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <memory>

using namespace std::string_literals;

std::string httpStatusCodeToString(int code);
std::string httpContentTypeToString(http::content_type type);

namespace http {

template <typename Arg, typename... Args> static void error(Arg &&arg, Args &&...args) {
	std::cerr << std::forward<Arg>(arg);
	((std::cerr << std::forward<Args>(args)), ...);
	std::cerr << std::endl;
}

template <typename Arg, typename... Args> static void log(Arg &&arg, Args &&...args) {
	std::cerr << std::forward<Arg>(arg);
	((std::cerr << std::forward<Args>(args)), ...);
	std::cerr << std::endl;
}

void handleRequest(int clientfd, server::requestCallbackType requestListener,
				   server::requestCallbackType dispatchInternalServerError);

exception::exception(int code, std::string message) : code(code), message(message) {}

std::ostream &operator<<(std::ostream &out, const ip &ip) {
	out << (int)ip._octets[0] << ".";
	out << (int)ip._octets[1] << ".";
	out << (int)ip._octets[2] << ".";
	out << (int)ip._octets[3];

	return out;
}

std::ostream &operator<<(std::ostream &out, const host &host) { return out << host.getIP(); }

ip::ip(uint32_t full) : _full(full) {}

url::url(std::string href) : _href(std::move(href)) {}

request::request(int clientfd, ::http::method method, const ::http::url &url)
	: method(method), url(url), _response(clientfd) {}

response &request::response() { return _response; }

response::response(int clientfd) : _clientfd(clientfd) {}

response::~response() { send(); }

int response::status() { return _status; }

size_t response::size() { return _content.length(); }

void response::setStatus(int status) { _status = status; }

void response::setHeader(const std::string &key, const std::string &value) { _headers[key] = value; }

void response::setContentType(const http::content_type content_type) { _content_type = content_type; }

void response::setContentString(const std::string &content) { _content = content; }

bool response::send() {
	std::string res;

	res += "HTTP/1.1 "s + httpStatusCodeToString(_status) + "\r\n"s;
	for (auto &[k, v] : _headers)
		res += k + ": " + v + "\r\n"s;
	res += "Content-Type: "s + httpContentTypeToString(_content_type) + "\r\n"s;
	res += "Content-Length: "s + std::to_string(_content.size()) + "\r\n"s;
	res += "\r\n"s;
	res += _content;

	return write(_clientfd, res.data(), res.length()) >= 0;
}

ip host::getIP() const {
	switch (_type) {
		case any:
			return ip(inet_addr("0.0.0.0"));
		case local:
			return ip(inet_addr("127.0.0.1"));
		default:
			throw "unreachable"s;
	}
}

server::server(requestCallbackType requestListener, requestCallbackType dispatchInternalServerError)
	: _requestListener(requestListener), _dispatchInternalServerError(dispatchInternalServerError) {}

template <typename T> T validate(T code) {
	if (code < 0)
		throw std::string(std::strerror(errno));
	return code;
}

void server::listen(const host &host, uint16_t port, std::function<void()> successCallback,
					std::function<void(const std::string &)> errorCallback) {
	try {
		_sockfd = validate(socket(PF_INET, SOCK_STREAM, IPPROTO_TCP));

		_serveraddr.sin_family = AF_INET;
		_serveraddr.sin_port = htons(port);
		_serveraddr.sin_addr.s_addr = host.getIP()._full;

		validate(bind(_sockfd, (sockaddr *)&_serveraddr, sizeof(_serveraddr)));

		validate(::listen(_sockfd, 1024));

		successCallback();
	} catch (const std::string &error) {
		errorCallback(error);
		return;
	}

	sockaddr_storage clientaddr;
	socklen_t clientsize = sizeof(clientaddr);

	char clientip[INET_ADDRSTRLEN];

	while (true) {
		int clientfd = accept(_sockfd, (sockaddr *)&clientaddr, &clientsize);
		inet_ntop(clientaddr.ss_family, (void *)&((sockaddr_in *)&clientaddr)->sin_addr, clientip, sizeof(clientip));
		handleRequest(clientfd, _requestListener, _dispatchInternalServerError);
	}
}

void handleRequest(int clientfd, server::requestCallbackType requestListener,
				   server::requestCallbackType dispatchInternalServerError) {

	std::string requestStr;
	struct {
		int code = 0;
		std::string message;
	} error;

	auto set_error = [&](const int code, const std::string message) {
		error.code = code;
		error.message = message;
	};

	{ // recieve request
		constexpr int BUFFER_SIZE = 256;
		char buffer[BUFFER_SIZE];

		std::string delimiter = "\r\n\r\n";
		size_t pos = std::string::npos;

		while ((pos = requestStr.find(delimiter)) == std::string::npos) {
			int bytesread = recv(clientfd, buffer, BUFFER_SIZE - 1, 0);
			if (bytesread <= 0) {
				::http::error("recv() failed: ", std::strerror(errno));
				return;
			}

			buffer[bytesread] = '\0';
			requestStr += buffer;
		}
	}

	struct {
		std::string method, url, protocolVersion;
		std::unordered_map<std::string, std::string> headers;
		std::unordered_map<std::string, std::string> payload; // POST-only

		const std::string getHeader(const std::string &key) {
			try {
				return headers.at(key);
			} catch (const std::exception &e) {
				return "_";
			}
		}
	} requestElements;

	{ // parse request
		std::istringstream requestStream(requestStr);

		std::string line;

		{ // parse request line
			std::getline(requestStream, line);
			std::istringstream requestLineStream(line);
			requestLineStream >> requestElements.method;
			requestLineStream >> requestElements.url;
			requestLineStream >> requestElements.protocolVersion;
		}

		{ // parse headers
			std::string header;
			while (std::getline(requestStream, line) && line != "\r") {
				std::istringstream headerStream(line);
				std::getline(headerStream, header, ':');
				std::string value;
				std::getline(headerStream, value);
				value.erase(0, value.find_first_not_of(' '));
				if (value[value.length() - 1] == '\r')
					value.pop_back();
				requestElements.headers[header] = value;
			}
		}

		{ // parse payload
			if (requestElements.method == "POST") {
				try {
					auto lenstr = requestElements.headers.at("Content-Length");
					auto len = stoi(lenstr);
					if (requestElements.headers.at("Content-Type") == "application/x-www-form-urlencoded") {
						std::string payload(len + 1, '\0');

						int bytesread = recv(clientfd, payload.data(), len, 0);
						if (bytesread <= 0) {
							::http::error("recv() failed: ", std::strerror(errno));
							return;
						}

						std::istringstream payloadStream(payload);
						std::string data;
						while (std::getline(payloadStream, data, '&')) {
							std::string key, value;
							std::istringstream dataStream(data);
							std::getline(dataStream, key, '=');
							std::getline(dataStream, value);
							requestElements.payload[key] = value;
						}
					} else {
						::http::error("incorrent content type: ");
					}
				} catch (const std::exception &e) {
					::http::error("exception caught: ", e.what());
				}
			}
		}
	}

	method req_method = method::UNKNOWN;
	std::shared_ptr<request> req;

	{
		static std::unordered_map<std::string, method> methodMap = {
			{"GET", method::GET},		  {"HEAD", method::HEAD},	  {"POST", method::POST},
			{"PUT", method::PUT},		  {"DELETE", method::DELETE}, {"CONNECT", method::CONNECT},
			{"OPTIONS", method::OPTIONS}, {"TRACE", method::TRACE},	  {"PATCH", method::PATCH},
		};

		try {
			req_method = methodMap.at(requestElements.method);
		} catch (const std::exception &e) {
			set_error(501, "The requested method '"s + requestElements.method + "' is not implemented by this server"s);
		}

		req = std::make_shared<request>(clientfd, req_method, requestElements.url);

		try {
			if (!requestListener(*req))
				throw "Request failed: "s + std::string(std::strerror(errno));
		} catch (const std::exception &err) {
			dispatchInternalServerError(*req);
		}
	}

	{ // close
		if (close(clientfd) < 0) {
			::http::error("close() returned: ", std::strerror(errno));
			return;
		}
	}

	::http::log(																	   // log message
		requestElements.getHeader("X-Forwarded-For"), "/",							   // IP/
		requestElements.getHeader("Cf-Ipcountry"), " ",								   // COUNTRY
		requestElements.getHeader("Sec-Ch-Ua-Platform"), " ",						   // "PLATFORM"
		requestElements.method, " ",												   // METHOD
		requestElements.getHeader("Host"), " ",										   // HOST
		requestElements.url, " ",													   // PATH
		req->response().status(), " ",												   // CODE
		(error.code ? error.message : (std::to_string(req->response().size()) + "b"s)) // SIZE or ERROR
	);
}

} // namespace http

std::string httpMethodToString(http::method method) {
	switch (method) {
		case http::method::GET:
			return "GET";
		case http::method::HEAD:
			return "HEAD";
		case http::method::POST:
			return "POST";
		case http::method::PUT:
			return "PUT";
		case http::method::DELETE:
			return "DELETE";
		case http::method::CONNECT:
			return "CONNECT";
		case http::method::OPTIONS:
			return "OPTIONS";
		case http::method::TRACE:
			return "TRACE";
		case http::method::PATCH:
			return "PATCH";
		case http::method::UNKNOWN:
		default:
			return "unknown";
	}
}

std::string httpStatusCodeToString(int code) {
	switch (code) {
		case 100:
			return "100 Continue";
		case 101:
			return "101 Switching Protocols";
		case 102:
			return "102 Processing";
		case 103:
			return "103 Early Hints";
		case 110:
			return "110 Response is Stale";
		case 111:
			return "111 Revalidation Failed";
		case 112:
			return "112 Disconnected Operation";
		case 113:
			return "113 Heuristic Expiration";
		case 199:
			return "199 Miscellaneous Warning";
		case 200:
			return "200 OK";
		case 201:
			return "201 Created";
		case 202:
			return "202 Accepted";
		case 203:
			return "203 Non-Authoritative Information";
		case 204:
			return "204 No Content";
		case 205:
			return "205 Reset Content";
		case 206:
			return "206 Partial Content";
		case 207:
			return "207 Multi-Status";
		case 208:
			return "208 Already Reported";
		case 214:
			return "214 Transformation Applied";
		case 226:
			return "226 IM Used";
		case 299:
			return "299 Miscellaneous Persistent Warning";
		case 300:
			return "300 Multiple Choices";
		case 301:
			return "301 Moved Permanently";
		case 302:
			return "302 Found";
		case 303:
			return "303 See Other";
		case 304:
			return "304 Not Modified";
		case 305:
			return "305 Use Proxy";
		case 306:
			return "306 Switch Proxy";
		case 307:
			return "307 Temporary Redirect";
		case 308:
			return "308 Permanent Redirect";
		case 400:
			return "400 Bad Request";
		case 401:
			return "401 Unauthorized";
		case 402:
			return "402 Payment Required";
		case 403:
			return "403 Forbidden";
		case 404:
			return "404 Not Found";
		case 405:
			return "405 Method Not Allowed";
		case 406:
			return "406 Not Acceptable";
		case 407:
			return "407 Proxy Authentication Required";
		case 408:
			return "408 Request Timeout";
		case 409:
			return "409 Conflict";
		case 410:
			return "410 Gone";
		case 411:
			return "411 Length Required";
		case 412:
			return "412 Precondition Failed";
		case 413:
			return "413 Payload Too Large";
		case 414:
			return "414 URI Too Long";
		case 415:
			return "415 Unsupported Media Type";
		case 416:
			return "416 Range Not Satisfiable";
		case 417:
			return "417 Expectation Failed";
		case 418:
			return "418 I'm a teapot";
		case 419:
			return "419 Page Expired";
		case 420:
			return "420 Method Failure";
		case 421:
			return "421 Misdirected Request";
		case 422:
			return "422 Unprocessable Entity";
		case 423:
			return "423 Locked";
		case 424:
			return "424 Failed Dependency";
		case 425:
			return "425 Too Early";
		case 426:
			return "426 Upgrade Required";
		case 428:
			return "428 Precondition Required";
		case 429:
			return "429 Too Many Requests";
		case 430:
			return "430 Request Header Fields Too Large";
		case 431:
			return "431 Request Header Fields Too Large";
		case 440:
			return "440 Login Time-out";
		case 444:
			return "444 No Response";
		case 449:
			return "449 Retry With";
		case 450:
			return "450 Blocked by Windows Parental Controls";
		case 451:
			return "451 Unavailable For Legal Reasons";
		case 494:
			return "494 Request header too large";
		case 495:
			return "495 SSL Certificate Error";
		case 496:
			return "496 SSL Certificate Required";
		case 497:
			return "497 HTTP Request Sent to HTTPS Port";
		case 498:
			return "498 Invalid Token";
		case 499:
			return "499 Token Required";
		case 500:
			return "500 Internal Server Error";
		case 501:
			return "501 Not Implemented";
		case 502:
			return "502 Bad Gateway";
		case 503:
			return "503 Service Unavailable";
		case 504:
			return "504 Gateway Timeout";
		case 505:
			return "505 HTTP Version Not Supported";
		case 506:
			return "506 Variant Also Negotiates";
		case 507:
			return "507 Insufficient Storage";
		case 508:
			return "508 Loop Detected";
		case 509:
			return "509 Bandwidth Limit Exceeded";
		case 510:
			return "510 Not Extended";
		case 511:
			return "511 Network Authentication Required";
		case 520:
			return "520 Web Server Returned an Unknown Error";
		case 521:
			return "521 Web Server Is Down";
		case 522:
			return "522 Connection Timed Out";
		case 523:
			return "523 Origin Is Unreachable";
		case 524:
			return "524 A Timeout Occurred";
		case 525:
			return "525 SSL Handshake Failed";
		case 526:
			return "526 Invalid SSL Certificate";
		case 527:
			return "527 Railgun Error";
		case 529:
			return "529 Site is overloaded";
		case 530:
			return "530 Site is frozen";
		case 561:
			return "561 Unauthorized";
		case 599:
			return "599 Network Connect Timeout Error";
	}

	throw "Undexpected response status code: "s + std::to_string(code);
}

std::string httpContentTypeToString(http::content_type type) {
	switch (type) {
		// case http::content_type::APPLICATION_JAVA_ARCHIVE: return "application/java-archive";
		// case http::content_type::APPLICATION_EDI_X12: return "application/EDI_X12";
		// case http::content_type::APPLICATION_EDIFACT: return "application/EDIFACT";
		// case http::content_type::APPLICATION_JAVASCRIPT: return "application/javascript";
		// case http::content_type::APPLICATION_OCTET_STREAM: return "application/octet-stream";
		// case http::content_type::APPLICATION_OGG: return "application/ogg";
		// case http::content_type::APPLICATION_PDF: return "application/pdf";
		// case http::content_type::APPLICATION_XHTML_XML: return "application/xhtml+xml";
		// case http::content_type::APPLICATION_X_SHOCKWAVE_FLASH: return "application/x-shockwave-flash";
		// case http::content_type::APPLICATION_JSON: return "application/json";
		// case http::content_type::APPLICATION_LD_JSON: return "application/ld+json";
		// case http::content_type::APPLICATION_XML: return "application/xml";
		// case http::content_type::APPLICATION_ZIP: return "application/zip";
		// case http::content_type::APPLICATION_X_WWW_FORM_URLENCODED: return "application/x-www-form-urlencoded";
		// case http::content_type::AUDIO_MPEG: return "audio/mpeg";
		// case http::content_type::AUDIO_X_MS_WMA: return "audio/x-ms-wma";
		// case http::content_type::AUDIO_VND_RN_REALAUDIO: return "audio/vnd.rn-realaudio";
		// case http::content_type::AUDIO_X_WAV: return "audio/x-wav";
		// case http::content_type::IMAGE_GIF: return "image/gif";
		// case http::content_type::IMAGE_JPEG: return "image/jpeg";
		// case http::content_type::IMAGE_PNG: return "image/png";
		// case http::content_type::IMAGE_TIFF: return "image/tiff";
		// case http::content_type::IMAGE_VND_MICROSOFT_ICON: return "image/vnd.microsoft.icon";
		// case http::content_type::IMAGE_X_ICON: return "image/x-icon";
		// case http::content_type::IMAGE_VND_DJVU: return "image/vnd.djvu";
		// case http::content_type::IMAGE_SVG_XML: return "image/svg+xml";
		// case http::content_type::TEXT_CSS: return "text/css";
		// case http::content_type::TEXT_CSV: return "text/csv";
		case http::content_type::TEXT_HTML:
			return "text/html";
		// case http::content_type::TEXT_JAVASCRIPT: return "text/javascript";
		case http::content_type::TEXT_PLAIN:
			return "text/plain";
			// case http::content_type::TEXT_XML: return "text/xml";
			// case http::content_type::VIDEO_MPEG: return "video/mpeg";
			// case http::content_type::VIDEO_MP4: return "video/mp4";
			// case http::content_type::VIDEO_QUICKTIME: return "video/quicktime";
			// case http::content_type::VIDEO_X_MS_WMV: return "video/x-ms-wmv";
			// case http::content_type::VIDEO_X_MSVIDEO: return "video/x-msvideo";
			// case http::content_type::VIDEO_X_FLV: return "video/x-flv";
			// case http::content_type::VIDEO_WEBM: return "video/webm";
	}

	throw "Undexpected content type: "s + std::to_string((int)type);
}