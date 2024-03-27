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
/// @file vrtlmodapi_source.cpp
/// @date Created on Wed Dec 09 13:32:12 2020
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vapi/generator.hpp"
#include "vrtlmod/core/core.hpp"
#include "vrtlmod/core/types.hpp"
#include "vrtlmod/util/logging.hpp"

#include <boost/algorithm/string/replace.hpp>

namespace vrtlmod
{
namespace vapi
{

std::string VapiGenerator::VapiSource::generate_body(void) const
{
    int td_nmb = 0;
    bool fast_compare = false;

    const auto &core = gen_.get_core();
    std::string top_name = core.get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4202
#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif
    std::string top_type = top_name;

    std::stringstream x, entries;

    std::string api_name = top_type + "VRTLmodAPI";

    x << R"(// vRTL-specific includes:
#include ")"
      << core.get_vrtltopheader_filename() << R"("
#include ")"
      << core.get_vrtltopsymsheader_filename() << R"("
// General API includes:
#include <memory>
#include <iostream>
#include "verilated.h"
#include ")"
      << gen_.get_targetdictionary_relpath() << R"("
#include ")"
      << gen_.get_apiheader_filename() << R"("

)" << api_name
      << "::" << api_name << R"((const char* name)
    : vrtlfi::td::TD_API()
    , vrtl_(name)
{
)";

    auto write_init_td = [&](const types::Module &M) -> bool
    {
        types::Module const *m = &M;
        types::Cell const *c = nullptr;

        auto celliter = [&](const types::Cell &C) -> bool
        {
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

        auto targetiterf = [&](const types::Target &t) -> bool
        {
            if (t.get_parent() == *m)
            {
                for (const auto &module_instance : t.get_parent().symboltable_instances_)
                {
                    std::stringstream smart_type, initializer;
                    std::string type_str, initializer_str;
                    auto prefix_str = core.get_prefix(c, module_instance);
                    auto member_str = util::concat("vrtl_.", core.get_memberstr(c, t, prefix_str));

                    std::string map_key = util::concat(prefix_str, ".", t.get_id());

                    smart_type << "vrtlfi::td::";

                    auto cxxdim = t.get_cxx_dimension_lengths();
                    auto one_dim_bits = t.get_one_dim_bits();
                    auto cxxdimtypes = t.get_cxx_dimension_types();

                    switch (cxxdim.size())
                    {
                    case 0:
                        smart_type << "ZeroD_TDentry<" << t.get_cxx_type() << ">";
                        initializer << "(\"" << prefix_str << "." << t.get_id() << "\", " << member_str << ", "
                                    << t.get_bits() << ", " << one_dim_bits << ")";
                        break;
                    case 1:
                        smart_type << "OneD_TDentry<" << t.get_cxx_type() << ", " << cxxdimtypes.back() << ", "
                                   << cxxdim[0] << ">";
                        initializer << "(\"" << prefix_str << "." << t.get_id() << "\", " << member_str << ", "
                                    << t.get_bits() << ", " << one_dim_bits << ")";
                        break;
                    case 2:
                        smart_type << "TwoD_TDentry<" << t.get_cxx_type() << ", " << cxxdimtypes.back() << ", "
                                   << cxxdim[0] << ", " << cxxdim[1] << ">";
                        initializer << "(\"" << prefix_str << "." << t.get_id() << "\", " << member_str << ", "
                                    << t.get_bits() << ", " << one_dim_bits << ")";
                        break;
                    case 3:
                        smart_type << "ThreeD_TDentry<" << t.get_cxx_type() << ", " << cxxdimtypes.back() << ", "
                                   << cxxdim[0] << ", " << cxxdim[1] << ", " << cxxdim[2] << ">";
                        initializer << "(\"" << prefix_str << "." << t.get_id() << "\", " << member_str << ", "
                                    << t.get_bits() << ", " << one_dim_bits << ")";
                        break;
                    default:
                        LOG_ERROR("CType dimensions of injection target not supported: ", t.get_cxx_type());
                        break;
                    }
                    x << "    td_[ \"" << map_key << "\" ]"
                      << " = "
                      << "std::make_shared< " << smart_type.str() << " >" << initializer.str() << ";\n";
                    x << "    " << member_str << "__td_"
                      << " = std::static_pointer_cast< " << smart_type.str() << ">(td_.at(\"" << map_key
                      << "\")).get();\n\n";
                }
            }
            return true;
        };
        core.foreach_injection_target(targetiterf);

        return true;
    };

    td_nmb = 0;
    core.foreach_module(write_init_td);

    if (core.is_systemc())
    {
        if (auto top_module = core.get_module_from_cell(core.get_top_cell()))
        {
            for (auto const &var : top_module->variables_)
            {
                std::string name = var->get_type(); //< seems odd to get name by get_type, but the name is the XML type where id is the XML node type
                if (name == "in" || name == "out" || name == "inout")
                {
                    auto type = var->get_bases();
                    util::strhelp::replace(type, "[", "");
                    util::strhelp::replace(type, "]", "");
                    auto port_type_str = type; //util::concat("sc_", name, "<", type, ">");
                    auto port_name = var->get_id();
                    std::string prefix_str = "TOP";
                    std::string map_key = util::concat(prefix_str, ".", port_name);
                    x << R"(
    td_[")" << map_key << "\"]" << " = " << "std::make_shared< " << "vrtlfi::td::SystemC_Port_TDentry< "
                      << port_type_str << " >" << " >" << "(" << R"(
          /*name:*/    ")" << map_key << R"("
        , /*dataRef:*/ vrtl_.)" << port_name << R"(
        , /*bits:*/    )" << var->get_bits()
                      << ")" << ";\n";
                }
            }
        }
    }
    x << R"(
)";

    std::string map_prefix = "";
    std::string retr_prefix = "";
    bool reverse_map = false;
    auto writetarget2idinitializer = [&](const types::Module &M) -> bool
    {
        types::Module const *m = &M;
        types::Cell const *c = nullptr;

        auto celliter = [&](const types::Cell &C) -> bool
        {
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

        auto targetiterf = [&](const types::Target &t) -> bool
        {
            if (t.get_parent() == *m)
            {
                for (const auto &module_instance : t.get_parent().symboltable_instances_)
                {
                    auto prefix_str = core.get_prefix(c, module_instance);
                    auto member_str = util::concat("vrtl_.", core.get_memberstr(c, t, prefix_str));
                    auto element_0 = member_str + "__td_";
                    auto element_1 = std::to_string(td_nmb);
                    x << "\n        ";
                    if (td_nmb == 0)
                    {
                        x << "  ";
                    }
                    else
                    {
                        x << ", ";
                    }
                    x << "{ " << (reverse_map ? element_1 : element_0) << ", " << (reverse_map ? element_0 : element_1)
                      << " }";
                    ++td_nmb;
                }
            }
            return true;
        };
        core.foreach_injection_target(targetiterf);
        return true;
    };
    auto write_systemc_target2id = [&](void) -> void {
        if (core.is_systemc())
        {
            if (auto top_module = core.get_module_from_cell(core.get_top_cell()))
            {
                for (auto const &var : top_module->variables_)
                {
                    std::string name = var->get_type(); //< seems odd to get name by get_type, but the name is the XML type where id is the XML node type
                    if (name == "in" || name == "out" || name == "inout")
                    {
                        auto port_name = var->get_id();
                        std::string prefix_str = "TOP";
                        auto element_0 = util::concat("td_.at(\"", prefix_str, ".", port_name, "\").get()");
                        auto element_1 = std::to_string(td_nmb);
                        x << "\n        ";
                        if (td_nmb == 0)
                        {
                            x << "  ";
                        }
                        else
                        {
                            x << ", ";
                        }
                        x << "{ " << (reverse_map ? element_1 : element_0) << ", " << (reverse_map ? element_0 : element_1)
                        << " }";
                        ++td_nmb;
                    }
                }
            }
        }
    };

    map_prefix = "";
    retr_prefix = "";
    reverse_map = true;
    td_nmb = 0;
    x << "    " << map_prefix << (reverse_map ? "id2target_ = {" : "target2id_ = {");
    core.foreach_module(writetarget2idinitializer);
    write_systemc_target2id();
    x << R"(
    };
)";

    map_prefix = "";
    retr_prefix = "";
    reverse_map = false;
    td_nmb = 0;
    x << "    " << map_prefix << (reverse_map ? "id2target_ = {" : "target2id_ = {");
    core.foreach_module(writetarget2idinitializer);
    write_systemc_target2id();
    x << R"(
    };
)";
    x << R"(
}

)";

    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
