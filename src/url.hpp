#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace http {

class url {
  public:
	url(std::string proto, std::string host, std::string uri);

	// As per RFC3986 specification https://datatracker.ietf.org/doc/html/rfc3986
	// but only for cloudflare hosted sites (eg. port and hash not included)
	//
	//                                                      search
	//                                              ┌─────────┴────────┐
	//                                              │    ┌───────────────searchparams["foo"]
	//                                              │    │             │
	//                                path          │    │       ┌───────searchparams["bar"]
	//                     ┌───────────┴───────────┐│    │       │     │
	// protocol://hostname/path[0]/path[1]/path[...]?foo=sth&bar=sthelse
	// └───────────────────────────────┬───────────────────────────────┘
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

}; // url

} // namespace http
