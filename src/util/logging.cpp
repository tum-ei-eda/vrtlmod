////////////////////////////////////////////////////////////////////////////////
/// @file logging.cpp
/// @author ?
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/util/logging.hpp"
#include "vrtlmod/transform/consumer.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace util {

namespace logging {

const char* toString(LEVEL level) {
	switch (level) {
	case OBLIGAT:
		return "\033[0;36m";
	case INFO:
		return "\033[1;37mInfo ";
	case WARNING:
		return "\033[1;33mWarning ";
	case ERROR:
		return "\033[0;31mError ";
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


template<> std::string toLogString<transform::Consumer>(const transform::Consumer &cons) {
	return std::string("{transform::Consumer file=") + cons.fc_.file_ + "}";
}

template<> std::string toLogString<std::tuple<const clang::SourceRange&, clang::SourceManager&> >(
		const std::tuple<const clang::SourceRange&, clang::SourceManager&> &cons) {
	return std::string("{clang::SourceRange {")
			+ toLogString(std::tie<const clang::SourceLocation&, clang::SourceManager&>(std::get<0>(cons).getBegin(), std::get<1>(cons)))
			+ ","
			+ toLogString(std::tie<const clang::SourceLocation&, clang::SourceManager&>(std::get<0>(cons).getEnd(), std::get<1>(cons)))
			+ "}";
}

template<> std::string toLogString<std::tuple<const clang::SourceLocation&, clang::SourceManager&> >(
		const std::tuple<const clang::SourceLocation&, clang::SourceManager&> &cons) {
	std::stringstream ss;
	{
		llvm::raw_os_ostream rout(ss);
		std::get<0>(cons).print(rout, std::get<1>(cons));
		rout.flush();
	}
	return ss.str();
}

template<> inline
std::string toLogString<std::tuple<clang::SourceManager &,const clang::SourceRange &> >(const std::tuple< clang::SourceManager &,const clang::SourceRange &> & cons)
{
	return toLogString<std::tuple<const clang::SourceRange &, clang::SourceManager &> >(std::tie(std::get<1>(cons),std::get<0>(cons)));
}

} // namespace util::logging

} // namespace util
