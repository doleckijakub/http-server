#pragma once

#include <functional>
#include <cstddef>
#include <string>

// OS-SPECIFIC INCLUDES TODO: CHANGE WHEN IMPLEMENTING CROSS-PLATFORM SUPPORT
#include <netinet/ip.h>

#include "host.hpp"
#include "request.hpp"

namespace http {

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
}; // server

} // namespace http
