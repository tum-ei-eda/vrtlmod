////////////////////////////////////////////////////////////////////////////////
/// @file Misc.cpp
////////////////////////////////////////////////////////////////////////////////

#include "ftcv/Misc.h"

#include <cstdlib>
#include <iostream>
#include "vrtlmod.hpp"

extern bool silent;

namespace ftcv {

const char* toString(LEVEL level) {
	switch (level) {
	case OBLIGAT:
		return "\033[0;36m";
	case INFO:
		return "\033[1;37mInfo";
	case WARNING:
		return "\033[1;33mWarning";
	case ERROR:
		return "\033[0;31mError";
	default:
		return "Unknown";
	}
}

void log(LEVEL level, const std::string &msg) {
	if(silent == true){
		if (level == ERROR or level == OBLIGAT)
			std::cout << toString(level) << ": " << "\033[0m" << msg << std::endl;
	}else{
		std::cout << toString(level) << ": " << "\033[0m" << msg << std::endl;
	}
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
