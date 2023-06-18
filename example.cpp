#include <iostream>

#include "http.hpp"

bool requestListener(http::request &req) {
	return req.respond_string(200, http::content_type::TEXT_PLAIN, "Hello, World!");
}

bool dispatchInternalServerError(http::request &req) {
	return req.respond_string(500, http::content_type::TEXT_PLAIN, "500 Internal server error");
}

int main(int argc, char const *argv[]) {
	uint16_t port = argc > 1 ? atoi(argv[1]) : 80;
	http::host host = http::host::local; // == 127.0.0.1, note http::host::any == 0.0.0.0

	http::server server(requestListener, dispatchInternalServerError);

	server.listen(
		host, port,
		[&]() {
			std::cerr << "Listening on " << host << ":" << port << std::endl;
		},
		[&](const std::string &error) {
			std::cerr << "Failed to listen: " << error << std::endl;
		});
}