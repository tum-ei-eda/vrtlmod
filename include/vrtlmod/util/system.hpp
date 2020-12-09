////////////////////////////////////////////////////////////////////////////////
/// @file system.hpp
/// @date Created on Mon Jan 10 18:21:20 2020
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_UTIL_SYSTEM_HPP__
#define __VRTLMOD_UTIL_SYSTEM_HPP__

#include <string>

////////////////////////////////////////////////////////////////////////////////
/// @brief General utility functions
namespace util {

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

} // namespace util::strhelp

////////////////////////////////////////////////////////////////////////////////
/// @brief System (and shell helper)
namespace system {
////////////////////////////////////////////////////////////////////////////////
/// @brief Execute a shell command and return shell response as string
/// @param cmd Command for shell
std::string exec(std::string cmd);

} // namespace util::system

} // namespace util



#endif /* __VRTLMOD_UTIL_SYSTEM_HPP__ */
