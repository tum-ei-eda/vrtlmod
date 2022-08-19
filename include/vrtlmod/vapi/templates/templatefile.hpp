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
/// @file templatefile.hpp
/// @date Created on Wed Dec 09 16:33:42 2020
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_VAPI_TEMPLATE_FILES_HPP__
#define __VRTLMOD_VAPI_TEMPLATE_FILES_HPP__

#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>

#include "vrtlmod/util/logging.hpp"

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all core vrtlmod functionalities
namespace vrtlmod
{
////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all API building functionalities
namespace vapi
{
////////////////////////////////////////////////////////////////////////////////
/// @class TemplateFile
/// @brief Self-contained base class for file generation without template files
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class TemplateFile
{
  protected:
    virtual std::string get_brief(void) const { return (std::string("")); }
    virtual std::string get_details(void) const { return (std::string("")); }
    virtual std::string get_date(void) const
    {
        std::time_t timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        return (std::ctime(&timestamp));
    }
    virtual std::string get_author(void) const { return (std::string("")); }

    virtual std::string generate_header(std::string filename) const
    {
        std::stringstream x;
        x << "////////////////////////////////////////////////////////////////////////////////\n\
/// @file "
          << filename << "\n\
/// @date "
          << get_date() << "\
/// @author "
          << get_author() << "\n\
/// @brief "
          << get_brief() << "\n\
/// @details "
          << get_details() << "\n\
////////////////////////////////////////////////////////////////////////////////\n"
          << std::endl;
        return x.str();
    };
    virtual std::string generate_body(void) const = 0;

  public:
    ////////////////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    TemplateFile(void) {}
    virtual ~TemplateFile(void){};

    void write(fs::path file_path) const
    {
        std::string fpathstr = file_path.string();
        auto filename = (fpathstr.rfind("/") != std::string::npos)    ? fpathstr.substr(fpathstr.rfind("/") + 1)
                        : (fpathstr.rfind("\\") != std::string::npos) ? fpathstr.substr(fpathstr.rfind("\\") + 1)
                                                                      : fpathstr;
        std::ofstream out;
        LOG_INFO("TemplateFile file path: [", fpathstr, "] open.");
        out.open(fpathstr);
        if (out.fail())
        {
            LOG_FATAL("TemplateFile file path: [", fpathstr, "] invalid.");
        }
        out << generate_header(filename);
        out << generate_body();
        out.close();
        LOG_INFO("TemplateFile file path: [", fpathstr, "] written.");
    }
};

} // namespace vapi
} // namespace vrtlmod

#endif /* __VRTLMOD_VAPI_TEMPLATE_FILES_HPP__ */
