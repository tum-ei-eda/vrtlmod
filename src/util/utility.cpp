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
#include <sstream>
#include <fstream>

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

std::string file2string(const fs::path &fpath)
{
    if(check_file(fpath) != 0)
    {
        LOG_FATAL("file2string(): File at [", fpath.c_str(), "] does not exist!");
        return "";
    }
    
    std::ifstream in(fpath.c_str());
    if (!in.is_open())
    {
        LOG_FATAL("file2string(): Could not open file path [", fpath.c_str(), "]");
        return "";
    }

    std::stringstream ss;

    while (in.good())
    {
        char c;
        in.get(c);
        ss << c;
    }
    in.close();

    return(ss.str());
}

void string2file(const fs::path &fpath, std::string const& data)
{
    std::ofstream out(fpath.c_str());
    if (!out.is_open())
    {
        LOG_FATAL("string2file(): Could not create file at [", fpath.c_str(), "]");
        return;
    }
    out << data;
    out.flush();
    out.close();
}

namespace strhelp
{

bool replace(std::string &str, const std::string &from, const std::string &to, size_t start_pos)
{
    size_t pos = str.find(from, start_pos);
    if (pos == std::string::npos)
        return false;
    str.replace(pos, from.length(), to);
    return true;
}

void replaceAll(std::string &str, const std::string &from, const std::string &to, size_t start_pos)
{
    size_t pos = start_pos;
    while(true)
    {
        pos = str.find(from, pos);
        if(pos == std::string::npos)
        {
            break;
        }
        str.replace(pos, from.size(), to);
        pos += to.size();
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
