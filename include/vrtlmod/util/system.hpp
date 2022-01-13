/*
 * Copyright 2021 Chair of EDA, Technical University of Munich
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
