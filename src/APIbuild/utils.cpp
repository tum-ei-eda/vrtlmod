////////////////////////////////////////////////////////////////////////////////
/// @file utils.cpp
/// @date Created on Mon Jan 26 18:21:20 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include <APIbuild/utils.hpp>
#include <ftcv/Misc.h>
#include <array>
#include <memory>

bool utils::strhelp::replace(std::string &str, const std::string &from, const std::string &to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

void utils::strhelp::replaceAll(std::string &str, const std::string &from, const std::string &to) {
	while (replace(str, from, to)) {
	}
}

std::string utils::system::exec(std::string cmd) {
	std::array<char, 128> buffer;
	std::string result;
	ftcv::log(ftcv::INFO, std::string("Executing shell command: \n\t" ).append(cmd));
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
	if (!pipe) {
		return ("");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	ftcv::log(ftcv::INFO, std::string(">: " ).append(result));
	return result;
}
