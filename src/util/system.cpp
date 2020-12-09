////////////////////////////////////////////////////////////////////////////////
/// @file system.cpp
/// @date Created on Mon Jan 26 18:21:20 2020
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/util/system.hpp"
#include "vrtlmod/util/logging.hpp"

#include <array>
#include <memory>

namespace util {

namespace strhelp {

bool replace(std::string &str, const std::string &from, const std::string &to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

void replaceAll(std::string &str, const std::string &from, const std::string &to) {
	while (replace(str, from, to)) {
	}
}

} // namespace util::strhelp


namespace system {

std::string exec(std::string cmd) {
	std::array<char, 128> buffer;
	std::string result;
	logging::log(logging::INFO, std::string("Executing shell command: \n\t" ).append(cmd));
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
	if (!pipe) {
		return ("");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	logging::log(logging::INFO, std::string(">: " ).append(result));
	return result;
}
} // namespace util::system

} // namespace util
