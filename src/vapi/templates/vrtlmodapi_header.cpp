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
#if VRTLMOD_VERILATOR_VERSION <= 4204
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
class )"
      << top_type << R"(;

struct )"
      << api_name << R"( : public vrtlfi::td::TD_API
{
)"
      << "    " << api_name << R"((const char* name =")" << top_type << R"(");
)"
      << "    virtual ~" << api_name << R"((void);
)"
      << "    " << api_name << "(" << api_name << R"( const&) = delete;
)"
      << "    void operator=(" << api_name << R"( const&) = delete;
)"
      << "    " << top_type << R"( vrtl_;
)"
      << R"(};

class )"

      << api_name << "Differential : private " << api_name << R"(
{
    std::map<const vrtlfi::td::TDentry*, size_t> reference_target2id_;
    std::map<const vrtlfi::td::TDentry*, size_t> faulty_target2id_;
    /////////////////////////////////////////////////////////////////////////////
    /// \brief Get link id for passed target which can be a pointer to an element
    ///        of either `faulty_`, `reference_`, or `this`
    size_t get_id(vrtlfi::td::TDentry const *target) const;
)";
    if (core.is_systemc())
    {
        if (auto top_module = core.get_module_from_cell(core.get_top_cell()))
        {
            for (auto const &var : top_module->variables_)
            {
                std::string name = var->get_type();
                if (name == "in" || name == "out" || name == "inout")
                {
                    auto type = var->get_bases();
                    util::strhelp::replace(type, "[", "");
                    util::strhelp::replace(type, "]", "");

                    auto port_name = var->get_id();
                    if (!util::strhelp::replace(type, "sc_out<", "sc_signal<"))
                        if (!util::strhelp::replace(type, "sc_in<", "sc_signal<"))
                            util::strhelp::replace(type, "sc_inout<", "sc_signal<");

                    x << R"(
    )" << type << " " << port_name
                      << "_dummy_{\"dummy_" << port_name << "\"};";
                    x << R"(
    )" << type << " " << port_name
                      << "_diff_{\"diff_" << port_name << "\"};";
                }
            }
        }
    }

    x << R"(
  public:
    std::map<const vrtlfi::td::TDentry*, size_t> diff_target2id_;)"
      << R"(
    )"
      << "const " << api_name << "& faulty_; ///< Fault injection core"
      << R"(
    const )"
      << api_name << "& reference_; ///< Reference core"
      << R"(

    /////////////////////////////////////////////////////////////////////////////
    /// \brief Calculate diff between `faulty_` and `reference_` target dict-
    ///        ionaries. Store diff (bitwise XOR) in )"
      << api_name << R"( base
    /// \return count of mismatching targets
    int diff_target_dictionaries(void);

    /////////////////////////////////////////////////////////////////////////////
    /// \brief Dump the Diff as CSV
    /// \param out Stream handle, may be fstream, sstream, cout, cerr, etc. ...
    void dump_diff_csv(std::ostream& out = std::cout) const;
    void dump_diff_csv_vertical(std::ostream& out = std::cout) const;

    /////////////////////////////////////////////////////////////////////////////
    /// \brief Compare `faulty_` with `reference_`.
    /// \param start Begin diff loop with target default or nullptr starts at
    ///              list entry..
    /// \return First mismatching target `vrtlfi::td::TDentry`.
    vrtlfi::td::TDentry const* compare_fast(vrtlfi::td::TDentry const *start = nullptr) const;

    /////////////////////////////////////////////////////////////////////////////
    /// \brief Constructor. Attach existing VrtlmodApis to this Differential
    )" << api_name
      << "Differential(const " << api_name << "& faulty, const " << api_name << R"(& reference);
)"
      << R"(
};

#endif /*__)"
      << top_type << "VRTLMODAPI_VRTLMODAPI_HPP__ */";

    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
