#pragma once

#include <cstddef>
#include <ostream>
#include <cstdint>

namespace http {

class server; // forward-declaration

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
}; // ip

} // namespace http
