#pragma once

#include <functional>
#include <ostream>

#ifdef __linux__
#include <netinet/ip.h>
#endif

namespace http {

class ip {
  public:
	ip();

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
	request(int clientfd, method method, url url);

	bool respond_string(int code, content_type content_type, const std::string &body);

	const method method;
	const url &url;

  private:
	int _clientfd;
};

class host {
  public:
	enum type {
		any,
		local,
	};

	constexpr host(type);

	ip getIP();

	friend std::ostream &operator<<(std::ostream &, const host &);

  private:
	type _type;
};

class server {
  public:
	using requestCallbackType = std::function<bool(request &)>;

	server(requestCallbackType requestListener, requestCallbackType dispatchInternalServerError);

	void listen(host host, uint16_t port, std::function<void()> successCallback,
				std::function<void(const std::string &)> errorCallback);

  private:
#ifdef __linux__
	int _sockfd;
	sockaddr_in _serveraddr;
#endif

	requestCallbackType _requestListener, _dispatchInternalServerError;
};

} // namespace http