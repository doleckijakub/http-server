#pragma once

#include <functional>
#include <ostream>

#include <netinet/ip.h>

namespace http {

class ip {
  public:
	ip(uint32_t);

	friend std::ostream &operator<<(std::ostream &, const ip &);

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
	GET,
	POST,
};

enum class content_type {
	TEXT_HTML,
	TEXT_PLAIN,
};

class request {
  public:
	request(int clientfd, ::http::method method, const ::http::url &url);

	bool respond_string(int code, content_type content_type, const std::string &body);

	const ::http::method method;
	const ::http::url &url;

  private:
	int _clientfd;
};

class host {
  public:
	enum type {
		any,
		local,
	};

	constexpr host(type type) : _type(type) {}

	friend std::ostream &operator<<(std::ostream &, const host &);

  private:
	ip getIP() const;
	type _type;
};

class server {
  public:
	using requestCallbackType = std::function<bool(request &)>;

	server(requestCallbackType requestListener, requestCallbackType dispatchInternalServerError);

	void listen(host host, uint16_t port, std::function<void()> successCallback,
				std::function<void(const std::string &)> errorCallback);

  private:
	int _sockfd;
	sockaddr_in _serveraddr;

	const requestCallbackType _requestListener, _dispatchInternalServerError;
};

} // namespace http