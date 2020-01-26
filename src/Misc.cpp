////////////////////////////////////////////////////////////////////////////////
/// @file Misc.cpp
////////////////////////////////////////////////////////////////////////////////

#include "ftcv/Misc.h"

#include <cstdlib>
#include <iostream>
#include "vrtlmod.hpp"
#include <sstream>


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

void log(LEVEL level, const std::string &msg, bool silent_toogle) {
	static bool silent = false;
	std::stringstream x;
	if(silent_toogle){
		silent = !silent;
	}
	x << toString(level);
	switch (level){
	case ERROR:
		x << msg << " \033[0m" << std::endl;
		break;
	case WARNING:
		x << msg << " \033[0m" << std::endl;
		break;
	default:
		x << " \033[0m" << msg << std::endl;
		break;
	}
	if(silent == true){
		if (level == ERROR or level == OBLIGAT)
			std::cout << x.str();
	}else{
		std::cout << x.str();
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
