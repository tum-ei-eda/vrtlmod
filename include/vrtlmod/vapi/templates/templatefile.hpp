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
/// @author Johannes Geier (johannes.geier@tum.de)
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
    ///////////////////////////////////////////////////////////////////////
    /// \brief Specified path to output directory
    std::string filepath_;
    std::string filename_;

  protected:
    std::string header_;
    std::string body_;

    virtual const std::string get_brief(void) { return (std::string("")); }
    virtual const std::string get_details(void) { return (std::string("")); }
    virtual const std::string get_date(void)
    {
        std::time_t timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        return (std::ctime(&timestamp));
    }
    virtual const std::string get_author(void) { return (std::string("")); }

    virtual void generate_header(void)
    {
        std::stringstream x;
        header_ = "";
        x << "////////////////////////////////////////////////////////////////////////////////\n\
/// @file "
          << filename_ << "\n\
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
        header_ = x.str();
    };
    virtual void generate_body(void) = 0;

  public:
    ///////////////////////////////////////////////////////////////////////
    /// \brief Returns output directory
    const std::string get_filepath(void) const { return (filepath_); }
    ///////////////////////////////////////////////////////////////////////
    /// \brief Returns file name
    const std::string get_filename(void) const { return (filename_); }

    ///////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    TemplateFile(void) {}
    virtual ~TemplateFile(void){};

    void write(const std::string filepath)
    {
        filepath_ = filepath;
        filename_ = (filepath_.rfind("/") != std::string::npos)    ? filepath_.substr(filepath_.rfind("/") + 1)
                    : (filepath_.rfind("\\") != std::string::npos) ? filepath_.substr(filepath_.rfind("\\") + 1)
                                                                   : filepath_;

        generate_header();
        generate_body();

        std::ofstream out;
        util::logging::log(util::logging::INFO,
                           std::string("TemplateFile file path: ") + filepath_ + std::string(" open"));
        out.open(filepath_);
        if (out.fail())
        {
            util::logging::abort(std::string("TemplateFile file path: ") + filepath_ + std::string(" invalid."));
        }
        out << header_;
        out << body_;
        out.close();
        util::logging::log(util::logging::INFO,
                           std::string("TemplateFile file path: ") + filepath_ + std::string(" written"));
    }
};

} // namespace vapi

#endif /* __VRTLMOD_VAPI_TEMPLATE_FILES_HPP__ */
