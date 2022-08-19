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
/// @file system.cpp
/// @date Created on Mon Jan 26 18:21:20 2020
////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <memory>

#include "vrtlmod/util/utility.hpp"
#include "vrtlmod/util/logging.hpp"

namespace util
{

int check_file(const fs::path &fpath)
{
    if (fs::is_regular_file(fpath) == false)
    {
        LOG_ERROR("File [", fpath.string(), "] does not exist in file system.");
        return 1; // not a valid file path
    }
    return 0;
}

namespace strhelp
{

bool replace(std::string &str, const std::string &from, const std::string &to)
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void replaceAll(std::string &str, const std::string &from, const std::string &to)
{
    while (replace(str, from, to))
    {
    }
}

} // namespace strhelp

namespace system
{

std::string exec(std::string cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    LOG_INFO("Executing shell command: \n\t", cmd);
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe)
    {
        return ("");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    LOG_INFO(">: ", result);
    return result;
}
} // namespace system

} // namespace util
