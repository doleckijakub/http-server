#pragma once

#include <iostream>

namespace http {

template <typename Arg, typename... Args> static void error(Arg &&arg, Args &&...args) {
	std::cerr << std::forward<Arg>(arg);
	((std::cerr << std::forward<Args>(args)), ...);
	std::cerr << std::endl;
}

template <typename Arg, typename... Args> static void log(Arg &&arg, Args &&...args) {
	std::cout << std::forward<Arg>(arg);
	((std::cout << std::forward<Args>(args)), ...);
	std::cout << std::endl;
}

} // namespace http