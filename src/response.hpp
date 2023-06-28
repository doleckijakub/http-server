#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>

#include "content_type.hpp"

namespace http {

namespace fs = std::filesystem;

class response {
  public:
	void setStatus(int status);
	void setHeader(const std::string &key, const std::string &value);
	void setContentType(const http::content_type content_type);
	void setContentString(const std::string &content);

	bool sendFile(fs::path filepath, const http::content_type content_type, const std::string displayPath);
	bool sendFile(fs::path filepath,
				  const std::string displayPath); // content type deduced from mime-type or extension,
												  // otherwise defaulted to application/octet-stream

	bool send();

	~response();

	friend class request;

	int status();
	size_t size();

  private:
	response(int clientfd);

	const int _clientfd;

	int _status = 200;
	std::unordered_map<std::string, std::string> _headers;
	http::content_type _content_type = http::content_type::TEXT_PLAIN;
	std::string _content;
}; // response

} // namespace http
