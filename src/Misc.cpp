////////////////////////////////////////////////////////////////////////////////
/// @file Misc.cpp
////////////////////////////////////////////////////////////////////////////////

#include "ftcv/Misc.h"

#include <cstdlib>
#include <iostream>

namespace ftcv {

const char* toString(LEVEL level) {
	switch (level) {
	case INFO:
		return "Info";
	case WARNING:
		return "Warning";
	case ERROR:
		return "Error";
	default:
		return "Unknown";
	}
}

void log(LEVEL level, const std::string &msg) {

	std::cout << toString(level) << ": " << msg << std::endl;

}

void abort() {
	std::abort();
}
void abort(const std::string &msg) {
	std::cerr << msg << std::endl;
	std::flush(std::cerr);
	abort();
}

} // namespace ftcv
