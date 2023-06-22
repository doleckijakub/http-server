#include "url.hpp"

using namespace std::string_literals;

static const std::string getPathname(const std::string &uri) {
	size_t questionMarkPos = uri.find('?');
	if (questionMarkPos != std::string::npos) {
		return uri.substr(0, questionMarkPos);
	} else {
		return uri;
	}
}

static const std::vector<std::string> getPath(const std::string &pathname) {
	std::vector<std::string> path;

	size_t start = 0;
	size_t end = pathname.find('/');

	while (end != std::string::npos) {
		std::string segment(pathname.c_str() + start, end - start);
		if (segment.length())
			path.push_back(segment);

		start = end + 1;
		end = pathname.find('/', start);
	}

	std::string lastSegment(pathname.c_str() + start);
	if (lastSegment.length())
		path.push_back(lastSegment);

	return path;
}

static const std::string getSearch(const std::string &uri) {
	size_t questionMarkPos = uri.find('?');
	if (questionMarkPos != std::string::npos) {
		return std::string(uri.c_str() + questionMarkPos);
	} else {
		return "";
	}
}

static std::unordered_map<std::string, std::string> getSearchParams(const std::string &search) {
	std::unordered_map<std::string, std::string> searchParams;

	auto parse = [&searchParams](const std::string &param) -> void { // TODO: unescape param
		size_t equalsPos = param.find('=');
		if (equalsPos != std::string::npos) {
			std::string key(param.data(), equalsPos);
			std::string value(param.data() + equalsPos + 1, param.length() - equalsPos - 1);

			searchParams[key] = value;
		} else {
			searchParams[param] = "";
		}
	};

	size_t start = 1;
	size_t end = search.find('&');

	while (end != std::string::npos) {
		if (start < end) {
			std::string param(search.data() + start, end - start);
			parse(param);

			start = end + 1;
			end = search.find('&', start);
		}
	}

	if (start < search.length()) {
		std::string lastParam(search.data() + start);
		parse(lastParam);
	}

	return searchParams;
}

template <class T> struct RemoveConst {
	typedef T type;
};

template <class T> struct RemoveConst<const T> {
	typedef T type;
};

namespace http {

#define unconst_ref(var) (*((RemoveConst<decltype(var)>::type *)&var))

url::url(std::string proto, std::string host, std::string uri) {
	unconst_ref(protocol) = proto + ":"s;
	unconst_ref(hostname) = host;
	unconst_ref(origin) = protocol + "//"s + hostname;
	unconst_ref(href) = origin + uri;

	unconst_ref(pathname) = getPathname(uri);
	unconst_ref(path) = getPath(pathname);

	unconst_ref(search) = getSearch(uri);
	unconst_ref(searchParams) = getSearchParams(search);
}

} // namespace http