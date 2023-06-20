#include <iostream>
#include <csignal>

#include "http.hpp"

bool requestListener(http::request &req) {
	req.response().setStatus(200);
	req.response().setContentType(http::content_type::TEXT_PLAIN);

	req.response().setContentString("Hello, World!");
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