#include "server.hpp"

#include <cstring>
#include <chrono>
#include <sstream>
#include <memory>

// OS-SPECIFIC INCLUDES TODO: CHANGE WHEN IMPLEMENTING CROSS-PLATFORM SUPPORT
#include <unistd.h>
#include <arpa/inet.h>

#include "log.hpp"
#include "exception.hpp"

template <typename T> T validate(T code) {
	if (code < 0)
		throw std::string(std::strerror(errno));
	return code;
}

using namespace std::string_literals;

namespace http {

static void handleRequest(int clientfd, server::requestCallbackType requestListener,
						  server::requestErrorCallbackType dispatchError); // forward-declaration

std::unordered_map<server *, std::pair<host, uint16_t>> server::_instances;

server::server(requestCallbackType requestListener, requestErrorCallbackType dispatchError)
	: _requestListener(requestListener), _dispatchError(dispatchError) {
}

void server::stopAllInstances(int) {
	std::putc('\n', stdout);

	for (const auto &[instance, ip] : _instances) {
		::http::log("Stopping ", ip.first, ":", ip.second, "... ",
					(instance->stop() ? "done"s : "failed: "s + std::string(std::strerror(errno))));
	}

	std::exit(0);
}

bool server::stop() {
	return close(_sockfd) == 0;
}

void server::listen(const host &host, uint16_t port, std::function<void()> successCallback,
					std::function<void(const std::string &)> errorCallback) {
	_instances.insert_or_assign(this, std::make_pair(host, port));

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

	while (true) {
		int clientfd = accept(_sockfd, (sockaddr *)&clientaddr, &clientsize);
		handleRequest(clientfd, _requestListener, _dispatchError);
	}
}

static void handleRequest(int clientfd, server::requestCallbackType requestListener,
						  server::requestErrorCallbackType dispatchError) {

	const auto startTime = std::chrono::high_resolution_clock::now();

	std::string requestStr;
	struct {
		int code = 0;
		std::string message;
	} error;

	auto set_error = [&](const int code, const std::string message) {
		error.code = code;
		error.message = message;
	};

	auto panic = [&](const std::string message) {
		error.code = -1;
		error.message = message + ": "s + std::strerror(errno);
	};

	auto panic_errno = [&](const std::string message) {
		panic(message + ": "s + std::strerror(errno));
	};

	{ // recieve request
		constexpr int BUFFER_SIZE = 256;
		char buffer[BUFFER_SIZE];

		std::string delimiter = "\r\n\r\n";
		size_t pos = std::string::npos;

		while ((pos = requestStr.find(delimiter)) == std::string::npos) {
			int bytesread = recv(clientfd, buffer, BUFFER_SIZE - 1, 0);
			if (bytesread <= 0) {
				panic_errno("Failed to recieve message from socket");
				break;
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

		const std::string getPlatform() {
			const std::string userAgent = getHeader("User-Agent");

			for (const auto &[agent, platform] : std::vector<std::pair<std::string, std::string>>({
					 {"Windows NT 10.0", "Windows 10"},
					 {"Windows NT 6.3", "Windows 8.1"},
					 {"Windows NT 6.2", "Windows 8"},
					 {"Windows NT 6.1", "Windows 7"},
					 {"Windows NT 6.0", "Windows Vista"},
					 {"Windows NT 5.2", "Windows Server 2003/XP x64"},
					 {"Windows NT 5.1", "Windows XP"},
					 {"Windows NT 5.01", "Windows 2000, Service Pack 1 (SP1)"},
					 {"Windows NT 5.0", "Windows 2000"},
					 {"Windows NT 4.0", "Windows NT 4.0"},
					 {"Windows NT", "Windows NT"},
				 })) {
				if (userAgent.find(agent) != std::string::npos)
					return platform;
			}

			for (const auto &platform : {
					 "Windows",	   "iPhone",	  "iPad",	 "Macintosh", "Mac OS X",	   "Mac_PowerPC", "Mac_68K",
					 "iOS",		   "Android",	  "FreeBSD", "OpenBSD",	  "NetBSD",		   "SunOS",		  "IRIX",
					 "HP-UX",	   "AIX",		  "OS/2",	 "QNX",		  "BeOS",		   "AmigaOS",	  "MorphOS",
					 "Nintendo",   "PlayStation", "Xbox",	 "Linux",	  "X11",		   "Chrome OS",	  "BlackBerry",
					 "Symbian OS", "PalmOS",	  "WebOS",	 "Tizen",	  "Windows Phone", "Windows CE",
				 }) {
				if (userAgent.find(platform) != std::string::npos)
					return platform;
			}

			return "_";
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
					auto contentType = requestElements.headers.at("Content-Type");
					if (contentType == "application/x-www-form-urlencoded") {
						std::string payload(len + 1, '\0');

						int bytesread = recv(clientfd, payload.data(), len, 0);
						if (bytesread <= 0) {
							panic_errno("Failed to recieve message from socket");
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
						set_error(501, "Parsing '"s + contentType + "' payloads not implemented by this server"s);
					}
				} catch (const std::exception &e) {
					set_error(400, "Bad request");
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

		url url(requestElements.getHeader("X-Forwarded-Proto"), requestElements.getHeader("Host"), requestElements.url);

		req = std::make_shared<request>(clientfd, req_method, url);

		if (error.code > 0) {
			dispatchError(*req, error.code, error.message);
		} else if (error.code != -1) {
			try {
				if (!requestListener(*req))
					throw exception(500, "Something went wrong");
			} catch (const exception &e) {
				dispatchError(*req, e.code, e.message);
			}
		}
	}

	{ // close
		if (close(clientfd) < 0) {
			panic_errno("Failed to close the socket");
		}
	}

	const auto endTime = std::chrono::high_resolution_clock::now();

	const static auto formatSize = [](const size_t bytes) -> std::string {
		const static size_t kilobyte = 1024;
		const static size_t megabyte = 1024 * 1024;
		const static size_t gigabyte = 1024 * 1024 * 1024;

		if (bytes < kilobyte) {
			return std::to_string(bytes) + "B";
		} else if (bytes < megabyte) {
			double sizeInKB = static_cast<double>(bytes) / kilobyte;
			return std::to_string(sizeInKB) + "KB";
		} else if (bytes < gigabyte) {
			double sizeInMB = static_cast<double>(bytes) / megabyte;
			return std::to_string(sizeInMB) + "MB";
		} else {
			double sizeInGB = static_cast<double>(bytes) / gigabyte;
			return std::to_string(sizeInGB) + "GB";
		}
	};

	const static auto executionTime = [&startTime, &endTime]() -> std::string {
		const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
		if (duration.count() < 1000) {
			return std::to_string(duration.count()) + "ms"s;
		} else if (duration.count() < 60000) {
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
			return std::to_string(seconds.count()) + "s"s;
		} else {
			auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration - minutes);
			return std::to_string(seconds.count()) + "min"s;
		}
	};

	const static auto responseOrError = [&]() -> std::string {
		if (error.code == -1)
			return error.message;
		return std::to_string(req->response().status()) + " "s + formatSize(req->response().size());
	};

	::http::log(										   // log message
		requestElements.getHeader("X-Forwarded-For"), "/", // IP/
		requestElements.getHeader("Cf-Ipcountry"), " ",	   // COUNTRY
		"(", requestElements.getPlatform(), ") ",		   // "PLATFORM"
		requestElements.method, " ",					   // METHOD
		requestElements.getHeader("Host"), " ",			   // HOST
		requestElements.url, " ",						   // PATH
		responseOrError(), " ",							   // RESPONSE (CODE AND SIZE) or ERROR
		executionTime()									   // EXECUTION TIME
	);
}

} // namespace http