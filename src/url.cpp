#include "url.hpp"

using namespace std::string_literals;

namespace http {

url::url(std::string proto, std::string host, std::string uri)
	: href(proto + "://"s + host + uri), protocol(proto + ":"), hostname(host), origin(proto + "://"s + host),
	  pathname(),	  // TODO: FINISH
	  path(),		  // TODO: FINISH
	  search(),		  // TODO: FINISH
	  searchparams(), // TODO: FINISH
	  hash()		  // TODO: FINISH
{}

} // namespace http