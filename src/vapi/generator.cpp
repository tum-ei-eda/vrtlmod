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
/// @file generator.cpp
/// @date Created on Mon Jan 07 14:12:11 2020
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vapi/generator.hpp"

#include "vrtlmod/core/core.hpp"
#include "vrtlmod/core/types.hpp"
#include "vrtlmod/util/utility.hpp"
#include "vrtlmod/util/logging.hpp"

#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iostream>

#include <pugixml.hpp>

#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;

namespace vrtlmod
{
namespace vapi
{

VapiGenerator::VapiGenerator(VrtlmodCore &vrtlmod_core) : core_(vrtlmod_core) {}

std::string VapiGenerator::get_targetdictionary_relpath(void) const
{
    std::string ret = API_TD_DIRPREFIX != "" ? std::string(API_TD_DIRPREFIX) + "/" : "";
    ret += get_targetdictionary_filename();
    return ret;
}
std::string VapiGenerator::get_apiheader_filename(void) const
{
    return util::concat(get_core().get_top_cell().get_type(), "_", API_HEADER_NAME);
}
std::string VapiGenerator::get_apisource_filename(void) const
{
    return util::concat(get_core().get_top_cell().get_type(), "_", API_SOURCE_NAME);
}

int VapiGenerator::build_targetdictionary(void) const
{
    auto api_dir = API_DIRPREFIX != "" ? get_core().get_output_dir() / API_DIRPREFIX : get_core().get_output_dir();
    if (!fs::exists(fs::path(api_dir)))
    {
        fs::create_directory(fs::path(api_dir));
    }
    tdheader_.write(api_dir / API_TD_HEADER_NAME);
    return 0;
}

int VapiGenerator::build_api(void) const
{
    auto api_dir = API_DIRPREFIX != "" ? get_core().get_output_dir() / API_DIRPREFIX : get_core().get_output_dir();

    if (!fs::exists(fs::path(api_dir)))
    {
        fs::create_directory(fs::path(api_dir));
    }
    vapisrc_.write(api_dir / get_apisource_filename());
    vapiheader_.write(api_dir / get_apiheader_filename());

    unsigned int failed = get_core().get_ctx().toinj_targets_.size();

    float perc = float(failed) / float(get_core().get_inj_targets().size()) * 100.0;
    LOG_OBLIGAT("Analysis vrtlmod run\n\t", "Uninjected Targets: ", std::to_string(failed), " of ",
                std::to_string(get_core().get_inj_targets().size()), " done. (", std::to_string(perc), "%)");

    return 0;
}

std::string VapiGenerator::getInludeStrings(void) const
{
    std::stringstream ret;
    ret << "/* Includes for Target Injection API */ \n\
#include \""
        << get_targetdictionary_relpath() << "\" \n\
#include \""
        << API_DIRPREFIX << "/" << get_apiheader_filename() << "\"\n";
    return (ret.str());
}

} // namespace vapi
} // namespace vrtlmod
