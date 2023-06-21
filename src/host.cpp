#include "host.hpp"

// OS-SPECIFIC INCLUDES TODO: CHANGE WHEN IMPLEMENTING CROSS-PLATFORM SUPPORT
#include <arpa/inet.h>

using namespace std::string_literals;

namespace http {

std::ostream &operator<<(std::ostream &out, const host &host) { return out << host.getIP(); }

ip host::getIP() const {
	switch (_type) {
		case any:
			return ip(inet_addr("0.0.0.0")); // TODO: DON'T USE inet_addr
		case local:
			return ip(inet_addr("127.0.0.1")); // TODO: DON'T USE inet_addr
		default:
			throw "unreachable"s;
	}
}

} // namespace http