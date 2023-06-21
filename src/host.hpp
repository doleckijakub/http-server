#pragma once

#include <ostream>

#include "ip.hpp"

namespace http {

class server; // forward-declaration

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
}; // host

} // namespace http
