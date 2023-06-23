#pragma once

#include <iostream>

namespace http {

template <typename Arg, typename... Args> static void warn(Arg &&arg, Args &&...args) {
	std::cout << "warning: " << std::forward<Arg>(arg);
	((std::cout << std::forward<Args>(args)), ...);
	std::cout << std::endl;
}

template <typename Arg, typename... Args> static void info(Arg &&arg, Args &&...args) {
	std::cout << "info: " << std::forward<Arg>(arg);
	((std::cout << std::forward<Args>(args)), ...);
	std::cout << std::endl;
}

template <typename Arg, typename... Args> static void log(Arg &&arg, Args &&...args) {
	std::cout << std::forward<Arg>(arg);
	((std::cout << std::forward<Args>(args)), ...);
	std::cout << std::endl;
}

} // namespace http