#include "response.hpp"

#include <fstream>
#include <sstream>

// OS-SPECIFIC INCLUDES TODO: CHANGE WHEN IMPLEMENTING CROSS-PLATFORM SUPPORT
#include <unistd.h>

#include "exception.hpp"
#include "log.hpp"

std::string httpStatusCodeToString(int code);				  // forward-declaration
std::string httpContentTypeToString(http::content_type type); // forward-declaration

using namespace std::string_literals;

namespace http {

response::response(int clientfd) : _clientfd(clientfd) {
}

response::~response() {
	send();
}

int response::status() {
	return _status;
}

size_t response::size() {
	return _content.length();
}

void response::setStatus(int status) {
	_status = status;
}

void response::setHeader(const std::string &key, const std::string &value) {
	_headers[key] = value;
}

void response::setContentType(const http::content_type content_type) {
	_content_type = content_type;
}

void response::setContentString(const std::string &content) {
	_content = content;
}

bool response::send() {
	std::string res;

	res += "HTTP/1.1 "s + httpStatusCodeToString(_status) + "\r\n"s;
	for (auto &[k, v] : _headers)
		res += k + ": " + v + "\r\n"s;
	res += "Content-Type: "s + httpContentTypeToString(_content_type) + "\r\n"s;
	res += "Content-Length: "s + std::to_string(_content.size()) + "\r\n"s;
	res += "\r\n"s;
	res += _content;

	return write(_clientfd, res.data(), res.length()) >= 0;
}

content_type getContentType(const fs::path filepath) {
	{ // check mime type
		static std::unordered_map<std::string, content_type> mimeTypeToContentTypeMap({
			{"text/html", content_type::TEXT_HTML},
			{"text/css", content_type::TEXT_CSS},
			{"text/javascript", content_type::TEXT_JAVASCRIPT},
			{"application/octet-stream", content_type::APPLICATION_OCTET_STREAM},
			{"application/ogg", content_type::APPLICATION_OGG},
			{"application/pdf", content_type::APPLICATION_PDF},
			{"application/xhtml+xml", content_type::APPLICATION_XHTML_XML},
			{"application/x-shockwave-flash", content_type::APPLICATION_X_SHOCKWAVE_FLASH},
			{"application/json", content_type::APPLICATION_JSON},
			{"application/ld+json", content_type::APPLICATION_LD_JSON},
			{"application/xml", content_type::APPLICATION_XML},
			{"application/zip", content_type::APPLICATION_ZIP},
			{"application/x-www-form-urlencoded", content_type::APPLICATION_X_WWW_FORM_URLENCODED},
			{"audio/mpeg", content_type::AUDIO_MPEG},
			{"audio/x-ms-wma", content_type::AUDIO_X_MS_WMA},
			{"audio/vnd.rn-realaudio", content_type::AUDIO_VND_RN_REALAUDIO},
			{"audio/x-wav", content_type::AUDIO_X_WAV},
			{"image/gif", content_type::IMAGE_GIF},
			{"image/jpeg", content_type::IMAGE_JPEG},
			{"image/png", content_type::IMAGE_PNG},
			{"image/tiff", content_type::IMAGE_TIFF},
			{"image/vnd.microsoft.icon", content_type::IMAGE_VND_MICROSOFT_ICON},
			{"image/x-icon", content_type::IMAGE_X_ICON},
			{"image/vnd.djvu", content_type::IMAGE_VND_DJVU},
			{"image/svg+xml", content_type::IMAGE_SVG_XML},
			{"video/mpeg", content_type::VIDEO_MPEG},
			{"video/mp4", content_type::VIDEO_MP4},
			{"video/quicktime", content_type::VIDEO_QUICKTIME},
			{"video/x-ms-wmv", content_type::VIDEO_X_MS_WMV},
			{"video/x-msvideo", content_type::VIDEO_X_MSVIDEO},
			{"video/x-flv", content_type::VIDEO_X_FLV},
			{"video/webm", content_type::VIDEO_WEBM},
		});

		char buffer[256];
		std::ostringstream command;
		command << "file -b --mime-type " << filepath.string() << " 2>/dev/null";
		std::string result;

		FILE *pipe = popen(command.str().c_str(), "r");
		if (pipe) {
			while (!std::feof(pipe)) {
				if (fgets(buffer, static_cast<int>(std::size(buffer)), pipe))
					result += buffer;
			}

			pclose(pipe);
		}

		result.erase(result.find_last_not_of("\r\n") + 1);

		if (mimeTypeToContentTypeMap.find(result) != mimeTypeToContentTypeMap.end()) {
			const content_type type = mimeTypeToContentTypeMap.at(result);
			::http::info("The content-type of ", filepath, " deduced from mime type: ", httpContentTypeToString(type));
			return type;
		}
	}

	{ // check file extension
		static std::unordered_map<std::string, content_type> extensionToContentTypeMap({
			{".jar", content_type::APPLICATION_JAVA_ARCHIVE},
			{".x12", content_type::APPLICATION_EDI_X12},
			{".edi", content_type::APPLICATION_EDIFACT},
			{".js", content_type::APPLICATION_JAVASCRIPT},
			{".bin", content_type::APPLICATION_OCTET_STREAM},
			{".ogg", content_type::APPLICATION_OGG},
			{".pdf", content_type::APPLICATION_PDF},
			{".xhtml", content_type::APPLICATION_XHTML_XML},
			{".swf", content_type::APPLICATION_X_SHOCKWAVE_FLASH},
			{".json", content_type::APPLICATION_JSON},
			{".jsonld", content_type::APPLICATION_LD_JSON},
			{".xml", content_type::APPLICATION_XML},
			{".zip", content_type::APPLICATION_ZIP},
			{".form", content_type::APPLICATION_X_WWW_FORM_URLENCODED},
			{".mp3", content_type::AUDIO_MPEG},
			{".wma", content_type::AUDIO_X_MS_WMA},
			{".ra", content_type::AUDIO_VND_RN_REALAUDIO},
			{".wav", content_type::AUDIO_X_WAV},
			{".gif", content_type::IMAGE_GIF},
			{".jpeg", content_type::IMAGE_JPEG},
			{".png", content_type::IMAGE_PNG},
			{".tiff", content_type::IMAGE_TIFF},
			{".ico", content_type::IMAGE_VND_MICROSOFT_ICON},
			{".ico", content_type::IMAGE_X_ICON},
			{".djvu", content_type::IMAGE_VND_DJVU},
			{".svg", content_type::IMAGE_SVG_XML},
			{".css", content_type::TEXT_CSS},
			{".csv", content_type::TEXT_CSV},
			{".html", content_type::TEXT_HTML},
			{".js", content_type::TEXT_JAVASCRIPT},
			{".txt", content_type::TEXT_PLAIN},
			{".xml", content_type::TEXT_XML},
			{".mpeg", content_type::VIDEO_MPEG},
			{".mp4", content_type::VIDEO_MP4},
			{".mov", content_type::VIDEO_QUICKTIME},
			{".wmv", content_type::VIDEO_X_MS_WMV},
			{".avi", content_type::VIDEO_X_MSVIDEO},
			{".flv", content_type::VIDEO_X_FLV},
			{".webm", content_type::VIDEO_WEBM},
		});

		const std::string extension = filepath.extension().string();

		if (extensionToContentTypeMap.find(extension) != extensionToContentTypeMap.end()) {
			const content_type type = extensionToContentTypeMap.at(extension);
			::http::info("The content-type of ", filepath, " deduced from extension: ", httpContentTypeToString(type));
			return type;
		}
	}

	::http::warn("The content-type of ", filepath, " unknown, defaulting to application/octet-stream");
	return content_type::APPLICATION_OCTET_STREAM;
}

std::string readFileContents(const fs::path &filepath) {
	std::ifstream file(filepath.string(), std::ios::binary | std::ios::ate);
	if (!file.good()) {
		throw exception(500, "Internal server error");
	}

	auto fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	std::string buffer(fileSize, '\0');

	if (!file.read(buffer.data(), fileSize)) {
		throw exception(500, "Internal server error");
	}

	file.close();

	return buffer;
}

bool response::sendFile(fs::path filepath, const http::content_type content_type, const std::string displayPath) {
	try {
		if (fs::exists(filepath)) {
			if (fs::is_directory(filepath)) {
				filepath /= "index.html";
			}

			if (fs::exists(filepath)) {
				std::string buffer = readFileContents(filepath);

				setStatus(200);
				setContentType(content_type);
				setContentString(buffer);
				return send();
			}
		}
	} catch (const std::exception &error) {
		throw exception(500, "Internal server error");
	}

	throw exception(404, "Resource "s + displayPath + " not found"s);
}

bool response::sendFile(fs::path filepath, const std::string displayPath) {
	try {
		if (fs::exists(filepath)) {
			if (fs::is_directory(filepath)) {
				filepath /= "index.html";
			}

			if (fs::exists(filepath)) {
				std::string buffer = readFileContents(filepath);

				setStatus(200);
				setContentType(getContentType(filepath));
				setContentString(buffer);
				return send();
			}
		}
	} catch (const std::exception &error) {
		throw exception(500, "Internal server error");
	}

	throw exception(404, "Resource "s + displayPath + " not found"s);
}

} // namespace http

std::string httpContentTypeToString(http::content_type type) { // TODO: move to content_type.cpp
	switch (type) {
		case http::content_type::APPLICATION_JAVA_ARCHIVE:
			return "application/java-archive";
		case http::content_type::APPLICATION_EDI_X12:
			return "application/EDI_X12";
		case http::content_type::APPLICATION_EDIFACT:
			return "application/EDIFACT";
		case http::content_type::APPLICATION_JAVASCRIPT:
			return "application/javascript";
		case http::content_type::APPLICATION_OCTET_STREAM:
			return "application/octet-stream";
		case http::content_type::APPLICATION_OGG:
			return "application/ogg";
		case http::content_type::APPLICATION_PDF:
			return "application/pdf";
		case http::content_type::APPLICATION_XHTML_XML:
			return "application/xhtml+xml";
		case http::content_type::APPLICATION_X_SHOCKWAVE_FLASH:
			return "application/x-shockwave-flash";
		case http::content_type::APPLICATION_JSON:
			return "application/json";
		case http::content_type::APPLICATION_LD_JSON:
			return "application/ld+json";
		case http::content_type::APPLICATION_XML:
			return "application/xml";
		case http::content_type::APPLICATION_ZIP:
			return "application/zip";
		case http::content_type::APPLICATION_X_WWW_FORM_URLENCODED:
			return "application/x-www-form-urlencoded";
		case http::content_type::AUDIO_MPEG:
			return "audio/mpeg";
		case http::content_type::AUDIO_X_MS_WMA:
			return "audio/x-ms-wma";
		case http::content_type::AUDIO_VND_RN_REALAUDIO:
			return "audio/vnd.rn-realaudio";
		case http::content_type::AUDIO_X_WAV:
			return "audio/x-wav";
		case http::content_type::IMAGE_GIF:
			return "image/gif";
		case http::content_type::IMAGE_JPEG:
			return "image/jpeg";
		case http::content_type::IMAGE_PNG:
			return "image/png";
		case http::content_type::IMAGE_TIFF:
			return "image/tiff";
		case http::content_type::IMAGE_VND_MICROSOFT_ICON:
			return "image/vnd.microsoft.icon";
		case http::content_type::IMAGE_X_ICON:
			return "image/x-icon";
		case http::content_type::IMAGE_VND_DJVU:
			return "image/vnd.djvu";
		case http::content_type::IMAGE_SVG_XML:
			return "image/svg+xml";
		case http::content_type::TEXT_CSS:
			return "text/css";
		case http::content_type::TEXT_CSV:
			return "text/csv";
		case http::content_type::TEXT_HTML:
			return "text/html";
		case http::content_type::TEXT_JAVASCRIPT:
			return "text/javascript";
		case http::content_type::TEXT_PLAIN:
			return "text/plain";
		case http::content_type::TEXT_XML:
			return "text/xml";
		case http::content_type::VIDEO_MPEG:
			return "video/mpeg";
		case http::content_type::VIDEO_MP4:
			return "video/mp4";
		case http::content_type::VIDEO_QUICKTIME:
			return "video/quicktime";
		case http::content_type::VIDEO_X_MS_WMV:
			return "video/x-ms-wmv";
		case http::content_type::VIDEO_X_MSVIDEO:
			return "video/x-msvideo";
		case http::content_type::VIDEO_X_FLV:
			return "video/x-flv";
		case http::content_type::VIDEO_WEBM:
			return "video/webm";
	}

	throw http::exception(500, "Something went wrong");
}

std::string httpStatusCodeToString(int code) {
	switch (code) {
		case 100:
			return "100 Continue";
		case 101:
			return "101 Switching Protocols";
		case 102:
			return "102 Processing";
		case 103:
			return "103 Early Hints";
		case 110:
			return "110 Response is Stale";
		case 111:
			return "111 Revalidation Failed";
		case 112:
			return "112 Disconnected Operation";
		case 113:
			return "113 Heuristic Expiration";
		case 199:
			return "199 Miscellaneous Warning";
		case 200:
			return "200 OK";
		case 201:
			return "201 Created";
		case 202:
			return "202 Accepted";
		case 203:
			return "203 Non-Authoritative Information";
		case 204:
			return "204 No Content";
		case 205:
			return "205 Reset Content";
		case 206:
			return "206 Partial Content";
		case 207:
			return "207 Multi-Status";
		case 208:
			return "208 Already Reported";
		case 214:
			return "214 Transformation Applied";
		case 226:
			return "226 IM Used";
		case 299:
			return "299 Miscellaneous Persistent Warning";
		case 300:
			return "300 Multiple Choices";
		case 301:
			return "301 Moved Permanently";
		case 302:
			return "302 Found";
		case 303:
			return "303 See Other";
		case 304:
			return "304 Not Modified";
		case 305:
			return "305 Use Proxy";
		case 306:
			return "306 Switch Proxy";
		case 307:
			return "307 Temporary Redirect";
		case 308:
			return "308 Permanent Redirect";
		case 400:
			return "400 Bad Request";
		case 401:
			return "401 Unauthorized";
		case 402:
			return "402 Payment Required";
		case 403:
			return "403 Forbidden";
		case 404:
			return "404 Not Found";
		case 405:
			return "405 Method Not Allowed";
		case 406:
			return "406 Not Acceptable";
		case 407:
			return "407 Proxy Authentication Required";
		case 408:
			return "408 Request Timeout";
		case 409:
			return "409 Conflict";
		case 410:
			return "410 Gone";
		case 411:
			return "411 Length Required";
		case 412:
			return "412 Precondition Failed";
		case 413:
			return "413 Payload Too Large";
		case 414:
			return "414 URI Too Long";
		case 415:
			return "415 Unsupported Media Type";
		case 416:
			return "416 Range Not Satisfiable";
		case 417:
			return "417 Expectation Failed";
		case 418:
			return "418 I'm a teapot";
		case 419:
			return "419 Page Expired";
		case 420:
			return "420 Method Failure";
		case 421:
			return "421 Misdirected Request";
		case 422:
			return "422 Unprocessable Entity";
		case 423:
			return "423 Locked";
		case 424:
			return "424 Failed Dependency";
		case 425:
			return "425 Too Early";
		case 426:
			return "426 Upgrade Required";
		case 428:
			return "428 Precondition Required";
		case 429:
			return "429 Too Many Requests";
		case 430:
			return "430 Request Header Fields Too Large";
		case 431:
			return "431 Request Header Fields Too Large";
		case 440:
			return "440 Login Time-out";
		case 444:
			return "444 No Response";
		case 449:
			return "449 Retry With";
		case 450:
			return "450 Blocked by Windows Parental Controls";
		case 451:
			return "451 Unavailable For Legal Reasons";
		case 494:
			return "494 Request header too large";
		case 495:
			return "495 SSL Certificate Error";
		case 496:
			return "496 SSL Certificate Required";
		case 497:
			return "497 HTTP Request Sent to HTTPS Port";
		case 498:
			return "498 Invalid Token";
		case 499:
			return "499 Token Required";
		case 500:
			return "500 Internal Server Error";
		case 501:
			return "501 Not Implemented";
		case 502:
			return "502 Bad Gateway";
		case 503:
			return "503 Service Unavailable";
		case 504:
			return "504 Gateway Timeout";
		case 505:
			return "505 HTTP Version Not Supported";
		case 506:
			return "506 Variant Also Negotiates";
		case 507:
			return "507 Insufficient Storage";
		case 508:
			return "508 Loop Detected";
		case 509:
			return "509 Bandwidth Limit Exceeded";
		case 510:
			return "510 Not Extended";
		case 511:
			return "511 Network Authentication Required";
		case 520:
			return "520 Web Server Returned an Unknown Error";
		case 521:
			return "521 Web Server Is Down";
		case 522:
			return "522 Connection Timed Out";
		case 523:
			return "523 Origin Is Unreachable";
		case 524:
			return "524 A Timeout Occurred";
		case 525:
			return "525 SSL Handshake Failed";
		case 526:
			return "526 Invalid SSL Certificate";
		case 527:
			return "527 Railgun Error";
		case 529:
			return "529 Site is overloaded";
		case 530:
			return "530 Site is frozen";
		case 561:
			return "561 Unauthorized";
		case 599:
			return "599 Network Connect Timeout Error";
	}

	return std::to_string(code);
}