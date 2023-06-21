#include "exception.hpp"

namespace http {

exception::exception(int code, std::string message) : code(code), message(message) {}

} // namespace http