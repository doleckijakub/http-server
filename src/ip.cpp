#include "ip.hpp"

namespace http {

std::ostream &operator<<(std::ostream &out, const ip &ip) {
	out << (int)ip._octets[0] << ".";
	out << (int)ip._octets[1] << ".";
	out << (int)ip._octets[2] << ".";
	out << (int)ip._octets[3];

	return out;
}

ip::ip(uint32_t full) : _full(full) {}

} // namespace http