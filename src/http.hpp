#pragma once

#include <functional>
#include <ostream>

#include <netinet/ip.h>

namespace http {

struct exception {
	const int code;
	const std::string message;
	exception(int code, std::string message);
};

class ip {
  public:
	ip(uint32_t);

	friend std::ostream &operator<<(std::ostream &, const ip &);

	friend class server;

  private:
	union {
		uint8_t _octets[4];
		uint32_t _full;
	};
};

class url {
  public:
	url(std::string);

  private:
	std::string _href;
};

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
};

enum class content_type {
	TEXT_HTML,
	TEXT_PLAIN,
};

class response {
  public:
	void setStatus(int status);
	void setHeader(const std::string &key, const std::string &value);
	void setContentType(const http::content_type content_type);
	void setContentString(const std::string &content);
	bool send();

	~response();

	friend class request;

	int status();
	size_t size();

  private:
	response(int clientfd);

	const int _clientfd;

	int _status = 200;
	std::unordered_map<std::string, std::string> _headers;
	http::content_type _content_type = http::content_type::TEXT_PLAIN;
	std::string _content;
};

struct request {
	request(int clientfd, ::http::method method, const ::http::url &url);

	const ::http::method method;
	const ::http::url &url;

	::http::response &response();

  private:
	::http::response _response;
};

class host {
  public:
	enum type {
		any,
		local,
	};

	constexpr host(type type) : _type(type) {}

	friend std::ostream &operator<<(std::ostream &, const host &);

	friend class server;

  private:
	ip getIP() const;
	type _type;
};

class server {
  public:
	using requestCallbackType = std::function<bool(request &)>;

	server(requestCallbackType requestListener, requestCallbackType dispatchInternalServerError);

	void listen(const host &host, uint16_t port, std::function<void()> successCallback,
				std::function<void(const std::string &)> errorCallback);

  private:
	int _sockfd;
	sockaddr_in _serveraddr;

	const requestCallbackType _requestListener, _dispatchInternalServerError;
};

} // namespace http