#include <iostream>
#include <csignal>
#include <cstring>

#include "http.hpp"

using namespace std::string_literals;

bool requestListener(http::request &req) {
	if (req.url.path.size() == 1 && req.url.path[0] == "favicon.ico") {
		return req.response().sendFile("favicon.ico");
	}

	req.response().setStatus(200);
	req.response().setContentType(http::content_type::TEXT_PLAIN);

	std::string response;

	response += "req.url.href: '"s + req.url.href + "'\n"s;
	response += "req.url.protocol: '"s + req.url.protocol + "'\n"s;
	response += "req.url.hostname: '"s + req.url.hostname + "'\n"s;
	response += "req.url.origin: '"s + req.url.origin + "'\n"s;

	response += "req.url.pathname: '"s + req.url.pathname + "'\n"s;

	response += "req.url.path: ["s;
	size_t path_size = req.url.path.size();
	for (const auto &segment : req.url.path) {
		response += "\n\t'"s + segment + "'";
		if (--path_size)
			response += ","s;
	}
	if (req.url.path.size())
		response += "\n"s;
	response += "]\n"s;

	response += "req.url.search: '"s + req.url.search + "'\n"s;

	response += "req.url.searchParams: {"s;
	size_t searchParams_size = req.url.searchParams.size();
	for (const auto &[key, value] : req.url.searchParams) {
		response += "\n\t'"s + key + "':'"s + value + "'";
		if (--searchParams_size)
			response += ","s;
	}
	if (req.url.searchParams.size())
		response += "\n"s;
	response += "}\n"s;

	req.response().setContentString(response);
	return req.response().send();
}

bool dispatchError(http::request &req, int code, const std::string error) {
	req.response().setStatus(code);
	req.response().setContentType(http::content_type::TEXT_PLAIN);

	req.response().setContentString(error);
	return req.response().send();
}

int main(int argc, char const *argv[]) {
	uint16_t port = argc > 1 ? atoi(argv[1]) : 80;
	http::host host = http::host::local; // == 127.0.0.1, note http::host::any == 0.0.0.0

	http::server server(requestListener, dispatchError);

	for (const auto &sig : {SIGINT, SIGTERM, SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGBUS, SIGSYS, SIGPIPE})
		std::signal(sig, http::server::stopAllInstances);

	server.listen(
		host, port,
		[&]() {
			std::cerr << "Listening on " << host << ":" << port << std::endl;
		},
		[&](const std::string &error) {
			std::cerr << "Failed to listen: " << error << std::endl;
		});
}