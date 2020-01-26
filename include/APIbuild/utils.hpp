////////////////////////////////////////////////////////////////////////////////
/// @file utils.hpp
/// @date Created on Mon Jan 10 18:21:20 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_APIBUILD_UTILS_HPP_
#define INCLUDE_APIBUILD_UTILS_HPP_

#include <string>

////////////////////////////////////////////////////////////////////////////////
/// @brief General utility functions
namespace utils {

////////////////////////////////////////////////////////////////////////////////
/// @brief String helper
namespace strhelp {
////////////////////////////////////////////////////////////////////////////////
/// @brief Replace the first occurrence of a substring with another
/// @param str In-/Out string
/// @param from Substring of str to be replaced
/// @param to Replacement for to be replaced substring of str
/// @return True if a substring was replaced, false if not.
bool replace(std::string &str, const std::string &from, const std::string &to);
////////////////////////////////////////////////////////////////////////////////
/// @brief Replace all occurrences of a substring with another
/// @param str In-/Out string
/// @param from Substring of str to be replaced
/// @param to Replacement for to be replaced substring of str
void replaceAll(std::string &str, const std::string &from, const std::string &to);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief System (and shell helper)
namespace system {
////////////////////////////////////////////////////////////////////////////////
/// @brief Execute a shell command and return shell response as string
/// @param cmd Command for shell
std::string exec(std::string cmd);

}

}

#endif /* INCLUDE_APIBUILD_UTILS_HPP_ */
