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
/// @file vrtlmodapi_header.cpp
/// @date Created on Wed Dec 09 13:32:12 2020
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vapi/generator.hpp"
#include "vrtlmod/core/core.hpp"
#include "vrtlmod/core/types.hpp"

namespace vrtlmod
{
namespace vapi
{

std::string VapiGenerator::VapiHeader::generate_body(void) const
{
    const auto &core = gen_.get_core();

    std::string top_name = core.get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4202
#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif
    std::string top_type = top_name;
    std::stringstream x, entries;

    std::string api_name = top_type + "VRTLmodAPI";

    x << R"(#ifndef __)" << top_type << R"(VRTLMODAPI_VRTLMODAPI_HPP__
#define __)"
      << top_type << R"(VRTLMODAPI_VRTLMODAPI_HPP__
)"
      << R"(
#include ")"
      << gen_.get_targetdictionary_relpath() << R"("
#include ")"
      << top_type << R"(.h"
#include <iostream>
#include <list>

class )"
      << top_type << R"(;

struct )"
      << api_name << R"( : public vrtlfi::td::TD_API
{
    std::map<const vrtlfi::td::TDentry*, size_t> target2id_;
    //
    )" << api_name << R"((const char* name =")" << top_type << R"(");
    //
    virtual ~)" << api_name << R"((void) = default;
    //
    )" << api_name << "(" << api_name << R"( const&) = delete;
    //
    void operator=()" << api_name << R"( const&) = delete;
    //
    )" << top_type << R"( vrtl_;
    //
    void connect_vrtl2api(void);
    /////////////////////////////////////////////////////////////////////////////
    /// \brief Dump the Diff as CSV
    /// \param out Stream handle, may be fstream, sstream, cout, cerr, etc. ...
    void dump_diff_csv(std::ostream& out = std::cout) const;
    void dump_diff_csv_vertical(std::ostream& out = std::cout) const;
    )";
    std::map<size_t, const vrtlfi::td::TDentry*> id2target_;

)"
      << "    " << api_name << R"((const char* name =")" << top_type << R"(");
)"
      << "    virtual ~" << api_name << R"((void) = default;
)"
      << "    " << api_name << "(" << api_name << R"( const&) = delete;
)"
      << "    void operator=(" << api_name << R"( const&) = delete;
)"
      << "    " << top_type << R"( vrtl_;
)"
      << R"(
};

#endif /*__)"
      << top_type << "VRTLMODAPI_VRTLMODAPI_HPP__ */";

    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
