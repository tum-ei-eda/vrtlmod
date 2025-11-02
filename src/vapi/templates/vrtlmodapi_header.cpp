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

#include "vrtlmod/vapi/generator.hpp"
#include "vrtlmod/core/core.hpp"
#include "vrtlmod/core/types.hpp"

namespace vrtlmod
{
namespace vapi
{

std::string VapiGenerator::VapiHeader::generate_body(void) const
{
    int td_nmb = 0;
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
#include <list>
#include "verilated.h"

class )"
      << top_type << R"(;

struct )"
      << api_name << R"( : public vrtlfi::td::TD_API
{
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

    auto write_td_value_members_declarations = [&](const types::Module &M) -> bool {
        types::Module const *m = &M;
        types::Cell const *c = nullptr;

        auto celliter = [&](const types::Cell &C) -> bool {
            if (m == core.get_module_from_cell(C))
            {
                c = &C;
                return false;
            }
            return true;
        };
        core.foreach_cell(celliter);

        if (c == nullptr)
        {
            LOG_FATAL("Can not find parent Cell of Module ", m->get_id(), " [", m->get_name(), "]");
        }

        auto targetiterf = [&](const types::Target &t) -> bool {
            if (t.get_parent() == *m)
            {
                for (const auto &module_instance : t.get_parent().symboltable_instances_)
                {
                    auto prefix_str = core.get_prefix(c, module_instance);
                    std::vector<std::string> cxxdim{};
                    for (int const &it : t.get_cxx_dimension_lengths())
                    {
                        cxxdim.push_back(std::to_string(it));
                    }
                    auto cxxdimtypes = t.get_cxx_dimension_types();
                    std::string decl_str;
                    switch (cxxdim.size())
                    {
                    case 0:
                        decl_str = util::concat("vrtlfi::td::", "ZeroD_TDentry< ", t.get_cxx_type(), ">");
                        break;
                    case 1:
                        decl_str = util::concat("vrtlfi::td::", "OneD_TDentry< ", t.get_cxx_type(), ", ",
                                                cxxdimtypes.back(), ", ", cxxdim[0], " >");
                        break;
                    case 2:
                        decl_str = util::concat("vrtlfi::td::", "TwoD_TDentry< ", t.get_cxx_type(), ", ",
                                                cxxdimtypes.back(), ", ", cxxdim[0], ", ", cxxdim[1], " >");
                        break;
                    case 3:
                        decl_str =
                            util::concat("vrtlfi::td::", "ThreeD_TDentry< ", t.get_cxx_type(), ", ", cxxdimtypes.back(),
                                         ", ", cxxdim[0], ", ", cxxdim[1], ", ", cxxdim[2], " >");
                        break;
                    default:
                        LOG_ERROR("CType dimensions of injection target not supported: ", t.get_cxx_type());
                        break;
                    }
                    std::string member_name = util::concat(prefix_str, "__DOT__", t.get_id(), "_");
                    util::strhelp::replaceAll(member_name, ".", "__DOT__");
                    util::strhelp::replaceAll(member_name, "->", "__REF__");
                    x << decl_str << " " << member_name << R"(;
    )";
                }
            }
            return true;
        };
        core.foreach_injection_target(targetiterf);

        return true;
    };

    td_nmb = 0;
    core.foreach_module(write_td_value_members_declarations);

    if (core.is_systemc())
    {
        if (auto top_module = core.get_module_from_cell(core.get_top_cell()))
        {
            for (auto const &var : top_module->variables_)
            {
                std::string name = var->get_type(); //< seems odd to get name by get_type, but the name is the XML type
                                                    // where id is the XML node type
                if (name == "in" || name == "out" || name == "inout")
                {
                    std::string port_decl_type = var->get_bases();
                    util::strhelp::replace(port_decl_type, "[", "");
                    util::strhelp::replace(port_decl_type, "]", "");
                    std::string decl_name = util::concat("TOP", "__DOT__", var->get_id());
                    util::strhelp::replaceAll(decl_name, ".", "__DOT__");
                    util::strhelp::replaceAll(decl_name, "->", "__REF__");
                    x << R"(
    vrtlfi::td::SystemC_Port_TDentry< )"
                      << port_decl_type << " > " << decl_name << "_;";
                }
            }
        }
    }
    x << R"(
)";

    x << R"(
    //
    std::map<size_t, const vrtlfi::td::TDentry*> id2target_;
    std::map<const vrtlfi::td::TDentry*, size_t> target2id_;
)";

    x << R"(
};

#endif /*__)"
      << top_type << "VRTLMODAPI_VRTLMODAPI_HPP__ */";

    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
