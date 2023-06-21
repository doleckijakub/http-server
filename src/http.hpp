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
	url(std::string proto, std::string host, std::string uri);

	// As per RFC3986 specification https://datatracker.ietf.org/doc/html/rfc3986
	// but only for cloudflare hosted sites (eg. port not included)
	//
	//                                                      search
	//                                              ┌─────────┴────────┐
	//                                              │    ┌───────────────searchparams["foo"]
	//                                              │    │             │
	//                                path          │    │       ┌───────searchparams["bar"]
	//                     ┌───────────┴───────────┐│    │       │     │
	// protocol://hostname/path[0]/path[1]/path[...]?foo=sth&bar=sthelse#hash
	// └──────────────────────────────────┬─────────────────────────────────┘
	// │                 │               href
	// └────────┬────────┘
	//        origin

	const std::string href;

	const std::string protocol;

	const std::string hostname; // same as host, because cloudflare and thus no port

	const std::string origin;

	const std::string pathname;
	const std::vector<std::string> path; // TODO: create own class

	const std::string search;
	const std::unordered_map<std::string, std::string> searchparams; // TODO: create own class

	const std::string hash;
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
	using requestErrorCallbackType = std::function<bool(request &, int, const std::string &)>;

	server(requestCallbackType requestListener, requestErrorCallbackType dispatchError);

	void listen(const host &host, uint16_t port, std::function<void()> successCallback,
				std::function<void(const std::string &)> errorCallback);

	static void stopAllInstances(int);

	bool stop();

  private:
	int _sockfd;
	sockaddr_in _serveraddr;

	static std::unordered_map<server *, std::pair<host, uint16_t>> _instances;

	const requestCallbackType _requestListener;
	const requestErrorCallbackType _dispatchError;
};

} // namespace http