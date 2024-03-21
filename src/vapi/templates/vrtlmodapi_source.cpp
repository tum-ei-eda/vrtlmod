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
static const char *SYMBOLTABLE_NAME =
#if VRTLMOD_VERILATOR_VERSION <= 4202
    "__VlSymsp";
#else // VRTLMOD_VERILATOR_VERSION <= 4228
    "vlSymsp";
#endif

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

    x << R"(// Vrtl-specific includes:
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

    auto get_prefix = [&](types::Cell const *c, std::string module_instance) -> std::string
    {
        //#if VRTLMOD_VERILATOR_VERSION <= 4202
        //#else // VRTLMOD_VERILATOR_VERSION <= 4228
        //        return (*c == core.get_top_cell()) ? "rootp" : module_instance;
        //#endif
        return (*c == core.get_top_cell()) ? core.get_top_cell().get_id() : module_instance;
    };

    auto get_memberstr = [&](types::Cell const *c, const types::Target &t, const std::string &prefix) -> std::string
    {
        return util::concat(
#if VRTLMOD_VERILATOR_VERSION <= 4202
            SYMBOLTABLE_NAME, "->", prefix, (*c == core.get_top_cell()) ? "->" : "."
#else // VRTLMOD_VERILATOR_VERSION <= 4228
            "rootp->", SYMBOLTABLE_NAME, "->", prefix, "."
#endif
            ,
            t.get_id());
    };

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
                    auto prefix_str = get_prefix(c, module_instance);
                    auto member_str = util::concat("vrtl_.", get_memberstr(c, t, prefix_str));

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
                    auto prefix_str = get_prefix(c, module_instance);
                    auto member_str = util::concat("vrtl_.", get_memberstr(c, t, prefix_str));
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

    x << api_name << "Differential::" << api_name << "Differential(const " << api_name << "& faulty, const " << api_name
      << R"(& reference)
    : )"
      << api_name << "(\"Differential\")"
      << R"(
    , faulty_(faulty)
    , reference_(reference)
)";

    x << "{";

    if (core.is_systemc())
    {
        if (auto top_module = core.get_module_from_cell(core.get_top_cell()))
        {
            for (auto const &var : top_module->variables_)
            {
                std::string name = var->get_type();
                if (name == "in" || name == "out" || name == "inout")
                {
                    auto type = var->get_cxx_type();
                    auto port_name = var->get_id();
                    if (!util::strhelp::replace(type, "sc_out<", "sc_signal<"))
                        if (!util::strhelp::replace(type, "sc_in<", "sc_signal<"))
                            util::strhelp::replace(type, "sc_inout<", "sc_signal<");

                    x << R"(
    )"
                      << "vrtl_." << port_name << "(" << port_name << "_dummy_);";
                }
            }
        }
    }

    auto writetarget2idinit = [&](const types::Module &M) -> bool
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
                    auto prefix_str = get_prefix(c, module_instance);
                    auto member_str = get_memberstr(c, t, prefix_str);

                    auto lhs_str = "faulty_.vrtl_." + member_str;
                    auto rhs_str = "reference_.vrtl_." + member_str;
                    auto xor_str = "this->vrtl_." + member_str;

                    x << R"(
    // )" << prefix_str
                      << "." << t.get_id() << ":"
                      << R"(
    faulty_.target2id_[faulty_.get_target)"
                      << "(\"" << prefix_str << "." << t.get_id() << "\")] = " << td_nmb << R"(;
    reference_.target2id_[reference_.get_target)"
                      << "(\"" << prefix_str << "." << t.get_id() << "\")] = " << td_nmb << R"(;
    this->target2id_[get_target)"
                      << "(\"" << prefix_str << "." << t.get_id() << "\")] = " << td_nmb << R"(;
)";
                    ++td_nmb;
                }
            }
            return true;
        };
        core.foreach_injection_target(targetiterf);

        return true;
    };

    x << R"(
}
)";

    auto writecomparebody = [&](const types::Module &M) -> bool
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
                    auto prefix_str = get_prefix(c, module_instance);
                    auto member_str = get_memberstr(c, t, prefix_str);

                    auto cxxdim = t.get_cxx_dimension_lengths();

                    auto lhs_str = "faulty_.vrtl_." + member_str;
                    auto rhs_str = "reference_.vrtl_." + member_str;
                    auto xor_str = "this->vrtl_." + member_str;

                    if (!fast_compare)
                    {
                        x << R"(
    )";
                    }
                    else
                    {
                        x << R"(
        case )" << td_nmb << ": ";
                    }
                    x << "// " << prefix_str << "." << t.get_id() << ":";

                    switch (cxxdim.size())
                    {
                    case 0:
                    {
                        if (!fast_compare)
                        {
                            x << R"(
    )" << xor_str << " = (" << lhs_str
                              << " ^ " << rhs_str << ") & 0x" << std::hex << t.get_element_mask({}) << std::dec << ";";
                            x << R"(
    ret += )" << xor_str << "? 1 : 0;";
                        }
                        else
                        {
                            x << R"(
            if(__UNLIKELY(()" << lhs_str
                              << " ^ " << rhs_str << ") & 0x" << std::hex << t.get_element_mask({}) << std::dec << "))"
                              << R"(
                return faulty_.td_.at)"
                              << "(\"" << prefix_str << "." << t.get_id() << "\").get();";
                        }
                    }
                    break;
                    case 1:
                    {
                        for (size_t m = 0; m < cxxdim[0]; ++m)
                        {
                            if (!fast_compare)
                            {
                                x << R"(
    )" << xor_str << "[" << m << "]"
                                  << " = (" << lhs_str << "[" << m << "]"
                                  << " ^ " << rhs_str << "[" << m << "])"
                                  << " & 0x" << std::hex << t.get_element_mask({ m }) << std::dec << ";";
                                x << R"(
    ret += )" << xor_str << "[" << m
                                  << "] ? 1 : 0;";
                            }
                            else
                            {
                                x << R"(
            if(__UNLIKELY(()" << lhs_str
                                  << "[" << m << "]"
                                  << " ^ " << rhs_str << "[" << m << "]) & 0x" << std::hex << t.get_element_mask({ m })
                                  << std::dec << "))"
                                  << R"(
                return faulty_.td_.at)"
                                  << "(\"" << prefix_str << "." << t.get_id() << "\").get();";
                            }
                        }
                    }
                    break;
                    case 2:
                    {
                        for (size_t l = 0; l < cxxdim[1]; ++l)
                            for (size_t m = 0; m < cxxdim[0]; ++m)
                            {
                                if (!fast_compare)
                                {
                                    x << R"(
    )" << xor_str << "[" << m << "]"
                                      << "[" << l << "]"
                                      << " = (" << lhs_str << "[" << m << "]"
                                      << "[" << l << "]"
                                      << " ^ " << rhs_str << "[" << m << "]"
                                      << "[" << l << "]) & 0x" << std::hex << t.get_element_mask({ m, l }) << std::dec
                                      << ";";
                                    x << R"(
    ret += )" << xor_str << "[" << m << "]"
                                      << "[" << l << "] ? 1 : 0;";
                                }
                                else
                                {
                                    x << R"(
            if(__UNLIKELY(()" << lhs_str
                                      << "[" << m << "]"
                                      << "[" << l << "]"
                                      << " ^ " << rhs_str << "[" << m << "]"
                                      << "[" << l << "]) & 0x" << std::hex << t.get_element_mask({ m, l }) << std::dec
                                      << " ))"
                                      << R"(
                return faulty_.td_.at)"
                                      << "(\"" << prefix_str << "." << t.get_id() << "\").get();";
                                }
                            }
                    }
                    break;
                    case 3:
                    {
                        for (size_t k = 0; k < cxxdim[2]; ++k)
                            for (size_t l = 0; l < cxxdim[1]; ++l)
                                for (size_t m = 0; m < cxxdim[0]; ++m)
                                {
                                    if (!fast_compare)
                                    {
                                        x << R"(
    )" << xor_str << "[" << m << "]"
                                          << "[" << l << "]"
                                          << "[" << k << "]"
                                          << " = (" << lhs_str << "[" << m << "]"
                                          << "[" << l << "]"
                                          << "[" << k << "]"
                                          << " ^ " << rhs_str << "[" << m << "]"
                                          << "[" << l << "]"
                                          << "[" << k << "]) & 0x" << std::hex << t.get_element_mask({ m, l, k })
                                          << std::dec << ";";
                                        x << R"(
    ret += )" << xor_str << "[" << m << "]"
                                          << "[" << l << "]"
                                          << "[" << k << "] ? 1 : 0;";
                                    }
                                    else
                                    {
                                        x << R"(
            if(__UNLIKELY(()" << lhs_str << "["
                                          << m << "]"
                                          << "[" << l << "]"
                                          << "[" << k << "]"
                                          << " ^ " << rhs_str << "[" << m << "]"
                                          << "[" << l << "]"
                                          << "[" << k << "]) & 0x" << std::hex << t.get_element_mask({ m, l, k })
                                          << std::dec << "))"
                                          << R"(
                return faulty_.td_.at)"
                                          << "(\"" << prefix_str << "." << t.get_id() << "\").get();";
                                    }
                                }
                    }
                    break;
                    default:
                        LOG_ERROR("CType dimensions of injection target not supported: ", t.get_cxx_type());
                        break;
                    }
                    if (fast_compare)
                    {
                        x << R"(
        [[ fallthrough ]];)";
                    }
                    x << std::endl;
                    ++td_nmb;
                }
            }
            return true;
        };
        core.foreach_injection_target(targetiterf);

        return true;
    };

    x << R"(
)"
      << "int " << api_name << "Differential::diff_target_dictionaries(void)"
      << R"(
{
    int ret = 0;
)";

    td_nmb = 0;
    fast_compare = false;
    core.foreach_module(writecomparebody);

    if (core.is_systemc())
    {
        if (auto top_module = core.get_module_from_cell(core.get_top_cell()))
        {
            for (auto const &var : top_module->variables_)
            {
                std::string name = var->get_type();
                if (name == "in" || name == "out" || name == "inout")
                {

                    auto type = var->get_cxx_type();
                    auto port_name = var->get_id();
                    x << R"(
    )" << port_name << "_diff_ = faulty_.vrtl_."
                      << port_name << ".read() ^ reference_.vrtl_." << port_name << ".read();"
                      << R"(
    )";
                    if (type.find("sc_bv<") != std::string::npos)
                    {
                        x << "for(size_t i = 0; i < " << port_name << "_diff_.size() ; ++i)"
                          << R"(
        ret += )" << port_name
                          << "_diff_.get_word(i) ? 1 : 0;";
                    }
                    else
                    {
                        x << "ret += " << port_name << "_diff_ ? 1 : 0;";
                    }
                    ++td_nmb;
                }
            }
        }
    }

    x << R"(
    return ret;
}

)";

    x << R"(
)"
      << "vrtlfi::td::TDentry const*" << api_name
      << "Differential::compare_fast(vrtlfi::td::TDentry const *start) const"
      << R"(
{
    size_t id = get_id(start);
    bool break_ = (id == 0);

compare_rotate_switch_entry:
    switch(id)
    {
)";
    td_nmb = 0;
    fast_compare = true;
    core.foreach_module(writecomparebody);

    if (core.is_systemc())
    {
        if (auto top_module = core.get_module_from_cell(core.get_top_cell()))
        {
            for (auto const &var : top_module->variables_)
            {
                std::string name = var->get_type(); //< seems odd to get name by get_type, but the name is the XML type where id is the XML node type
                if (name == "in" || name == "out" || name == "inout")
                {
                    auto type = var->get_cxx_type();
                    auto port_name = var->get_id();
                    auto diff_expr = util::concat("faulty_.vrtl_.", port_name, ".read() ^ reference_.vrtl_.", port_name, ".read()");

                    x << R"(
        case )" << td_nmb << ": ";
                    x << R"(
            if(__UNLIKELY(()" << diff_expr << ") != 0))" << R"(
                return faulty_.td_.at("TOP.)" << port_name<< "\").get();";
                    x << R"(
        [[ fallthrough ]];)";
                    ++td_nmb;
                }
            }
        }
    }

    x << R"(
        default:
        {
            if (!break_)
            {
                id = 0;
                break_ = true;
                goto compare_rotate_switch_entry;
            }
        }
        break;
    }
    return nullptr;
}

)";

    x << R"(
size_t )"
      << api_name << R"(Differential::get_id(vrtlfi::td::TDentry const *target) const
{
    if(auto pos = faulty_.target2id_.find(target); pos != faulty_.target2id_.end())
    {
        return pos->second;
    }
    if(auto pos = reference_.target2id_.find(target); pos != reference_.target2id_.end())
    {
        return pos->second;
    }
    if(auto pos = this->target2id_.find(target); pos != this->target2id_.end())
    {
        return pos->second;
    }
    return 0;
}

)";

    x << R"(void )" << api_name << R"(Differential::dump_diff_csv(std::ostream& out) const
{
    for(auto const& it: this->target2id_)
    {
        out << (it.first)->get_name() << ", 0b";

        auto bitvector = (it.first)->read_data();
        for (auto it = bitvector.rbegin(); it != bitvector.rend(); ++it)
        {
            out << int(*it);
        }
        out << std::endl;
    }
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

                    auto type = var->get_cxx_type();
                    auto port_name = var->get_id();

                    x << R"(
    out << ")" << port_name
                      << "[" << name << "], 0b\";";
                    if (type.find("sc_bv<") != std::string::npos)
                    {
                        x << R"(
    for(size_t i = 0; i < )"
                          << port_name << "_diff_.length() ; ++i)"
                          << R"(
        out << )" << port_name
                          << R"(_diff_.get_bit(i) ? 1 : 0;
    out << std::endl;)";
                    }
                    else
                    {
                        x << R"(
    out << )" << port_name << "_diff_ << std::endl;";
                    }
                }
            }
        }
    }
    x << R"(
}

)";

    x << R"(void )" << api_name << R"(Differential::dump_diff_csv_vertical(std::ostream& out) const
{
    static bool header = true;
    if(header)
    {
        header = false;
        for(auto it = this->target2id_.begin(); it != this->target2id_.end(); ++it)
        {
            out << (it->first)->get_name() << ",";
        }
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

                    auto type = var->get_cxx_type();
                    auto port_name = var->get_id();

                    x << R"(
        out << ")" << port_name
                      << "[" << name << "],\";";
                }
            }
        }
    }
    x << R"(
        out << std::endl;
    }

    for(auto it = this->target2id_.begin(); it != this->target2id_.end(); ++it)
    {
        auto bitvector = (it->first)->read_data();
        for (auto bit = bitvector.rbegin(); bit != bitvector.rend(); ++bit)
        {
            out << int(*bit);
        }
        out << ",";
    }
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

                    auto type = var->get_cxx_type();
                    auto port_name = var->get_id();
                    x << R"(
    out << "0b";)";
                    if (type.find("sc_bv<") != std::string::npos)
                    {
                        x << R"(
    for(size_t i = 0; i < )"
                          << port_name << "_diff_.length() ; ++i)"
                          << R"(
        out << )" << port_name
                          << R"(_diff_.get_bit(i) ? 1 : 0;
    out << ",";)";
                    }
                    else
                    {
                        x << R"(
    out << )" << port_name << R"(_diff_ << ",";)";
                    }
                }
            }
        }
    }
    x << R"(
    out << std::endl;
}
)";

    auto writediffbody = [&](const types::Module &M) -> bool
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
                    size_t element_idx = 0;
                    auto prefix_str = get_prefix(c, module_instance);
                    auto member_str = get_memberstr(c, t, prefix_str);

                    auto cxxdim = t.get_cxx_dimension_lengths();

                    auto lhs_str = "faulty_.vrtl_." + member_str;
                    auto rhs_str = "reference_.vrtl_." + member_str;
                    auto xor_str = "this->vrtl_." + member_str;

                    x << R"(
        case )" << td_nmb
                      << ": ";
                    x << "// " << prefix_str << "." << t.get_id() << ":";

                    switch (cxxdim.size())
                    {
                    case 0:
                    {
                        x << R"(
        {
            (void)(element_idx) /* unused */;
            auto d = ()" << xor_str
                          << " ^ "
                          << "val"
                          << ") & 0x" << std::hex << t.get_element_mask({}) << std::dec << ";";
                        x << R"(
            return d == 0 ? true : false;
        })";
                    }
                    break;
                    case 1:
                    {
                        x << R"(
        {
            switch (element_idx)
            {)";
                        for (size_t m = 0; m < cxxdim[0]; ++m)
                        {
                            x << R"(
                case )" << element_idx
                              << ":"
                              << R"(
                {
                    auto d = ()"
                              << xor_str << "[" << m << "]"
                              << " ^ "
                              << "val"
                              << ") & 0x" << std::hex << t.get_element_mask({ m }) << std::dec << ";";
                            x << R"(
                    return d == 0 ? true : false;
                })";
                            ++element_idx;
                        }
                        x << R"(
            }
        })";
                    }
                    break;
                    case 2:
                    {
                        x << R"(
        {
            switch (element_idx)
            {)";
                        for (size_t l = 0; l < cxxdim[1]; ++l)
                            for (size_t m = 0; m < cxxdim[0]; ++m)
                            {
                                x << R"(
                case )" << element_idx
                                  << ":"
                                  << R"(
                {
                    auto d = ()" << xor_str
                                  << "[" << m << "]"
                                  << "[" << l << "]"
                                  << " ^ "
                                  << "val"
                                  << ") & 0x" << std::hex << t.get_element_mask({ m, l }) << std::dec << ";";
                                x << R"(
                    return d == 0 ? true : false;
                })";
                                ++element_idx;
                            }
                        x << R"(
            }
        })";
                    }
                    break;
                    case 3:
                    {
                        x << R"(
        {
            switch (element_idx)
            {)";
                        for (size_t k = 0; k < cxxdim[2]; ++k)
                            for (size_t l = 0; l < cxxdim[1]; ++l)
                                for (size_t m = 0; m < cxxdim[0]; ++m)
                                {
                                    x << R"(
                case )" << element_idx << ":"
                                      << R"(
                {
                    auto d = ()" << xor_str
                                      << "[" << m << "]"
                                      << "[" << l << "]"
                                      << "[" << k << "]"
                                      << " ^ "
                                      << "val"
                                      << ") & 0x" << std::hex << t.get_element_mask({ m, l, k }) << std::dec << ";";
                                    x << R"(
                    return d == 0 ? true : false;
                })";
                                    ++element_idx;
                                }
                        x << R"(
            }
        }
)";
                    }
                    break;
                    default:
                        LOG_ERROR("CType dimensions of injection target not supported: ", t.get_cxx_type());
                        break;
                    }
                    ++td_nmb;
                }
            }
            return true;
        };
        core.foreach_injection_target(targetiterf);

        return true;
    };

    x << R"(
)"
      << "bool " << api_name << "Differential::diff_target(size_t target_idx, size_t element_idx, uint64_t val) const"
      << R"(
{
    int ret = 0;
)";

    td_nmb = 0;
    x << R"(
    switch (target_idx)
    {)";
    
    fast_compare = false;
    core.foreach_module(writediffbody);

    if (core.is_systemc())
    {
        if (auto top_module = core.get_module_from_cell(core.get_top_cell()))
        {
            for (auto const &var : top_module->variables_)
            {
                std::string name = var->get_type();
                if (name == "in" || name == "out" || name == "inout")
                {

                    auto type = var->get_cxx_type();
                    auto port_name = var->get_id();

                    x << R"(
        case )" << td_nmb
                      << ": ";
                    x << "// " << top_module->get_id() << "." << port_name << ":";
                    x << R"(
        {)";
                    if (type.find("sc_bv<") == std::string::npos)
                    {
                        x << R"(
            auto d = ()" << port_name
                          << "_diff_"
                          << " ^ "
                          << "val"
                          << ");";
                        x << R"(
            return d == 0 ? true : false;
        })";
                    }
                    else
                    {
                        x << R"(
            auto d = ()" << port_name
                          << "_diff_.to_uint64()"
                          << " ^ "
                          << "val"
                          << ");";
                        x << R"(
            return d == 0 ? true : false;
        })";
                    }
                    ++td_nmb;
                }
            }
        }
    }

    x << R"(

        default:
            // TODO: we should maybe except here.
            break;
    })";

    x << R"(
    return false;
}
)";

    auto write_triplet_push = [&](const types::Module &M) -> bool
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
                    size_t element_idx = 0;
                    auto prefix_str = get_prefix(c, module_instance);
                    auto member_str = get_memberstr(c, t, prefix_str);

                    auto cxxdim = t.get_cxx_dimension_lengths();

                    auto lhs_str = "faulty_.vrtl_." + member_str;
                    auto rhs_str = "reference_.vrtl_." + member_str;
                    auto xor_str = "this->vrtl_." + member_str;

                    x << R"(
    // )" << prefix_str
                      << "." << t.get_id() << ":";

                    switch (cxxdim.size())
                    {
                    case 0:
                    {
                        x << R"(
    if (auto d = )"
                          << "static_cast<uint64_t>(" << xor_str << R"())
    {
        nz_triplet_list.push_back({ )"
                          << td_nmb << ", "
                          << "static_cast<uint16_t>(-1)"
                          << ", d });"
                          << R"(
    }
)";
                    }
                    break;
                    case 1:
                    {
                        auto element_ctr = 0;
                        for (size_t m = 0; m < cxxdim[0]; ++m)
                        {
                            x << R"(
    if (auto d = )"
                              << "static_cast<uint64_t>(" << xor_str << "[" << m << "]"
                              << R"())
    {
        nz_triplet_list.push_back({ )"
                              << td_nmb << ", " << element_ctr << ", d });"
                              << R"(
    }
)";
                            ++element_ctr;
                        }
                    }
                    break;
                    case 2:
                    {
                        auto element_ctr = 0;
                        for (size_t l = 0; l < cxxdim[1]; ++l)
                            for (size_t m = 0; m < cxxdim[0]; ++m)
                            {
                                x << R"(
    if (auto d = )"
                                  << "static_cast<uint64_t>(" << xor_str << "[" << m << "]"
                                  << "[" << l << "]"
                                  << R"())
    {
        nz_triplet_list.push_back({ )"
                                  << td_nmb << ", " << element_ctr << ", d });"
                                  << R"(
    }
)";
                                ++element_ctr;
                            }
                    }
                    break;
                    case 3:
                    {
                        auto element_ctr = 0;
                        for (size_t k = 0; k < cxxdim[2]; ++k)
                            for (size_t l = 0; l < cxxdim[1]; ++l)
                                for (size_t m = 0; m < cxxdim[0]; ++m)
                                {
                                    x << R"(
    if (auto d = )"
                                      << "static_cast<uint64_t>(" << xor_str << "[" << m << "]"
                                      << "[" << l << "]"
                                      << "[" << k << "]"
                                      << R"())
    {
        nz_triplet_list.push_back({ )" << td_nmb
                                      << ", " << element_ctr << ", d });"
                                      << R"(
    }
)";
                                    ++element_ctr;
                                }
                    }
                    break;
                    default:
                        LOG_ERROR("CType dimensions of injection target not supported: ", t.get_cxx_type());
                        break;
                    }
                    ++td_nmb;
                }
            }
            return true;
        };
        core.foreach_injection_target(targetiterf);

        return true;
    };

    auto N_targets = td_nmb;

    x << R"(
)"
      << "std::list<vrtlfi::td::UniqueElementTriplet> " << api_name << "Differential::get_non_zeros(void) const"
      << R"(
{
    std::list<vrtlfi::td::UniqueElementTriplet> nz_triplet_list{};
)";

    td_nmb = 0;
    core.foreach_module(write_triplet_push);

    if (core.is_systemc())
    {
        if (auto top_module = core.get_module_from_cell(core.get_top_cell()))
        {
            for (auto const &var : top_module->variables_)
            {
                std::string name = var->get_type();
                if (name == "in" || name == "out" || name == "inout")
                {

                    auto type = var->get_cxx_type();
                    auto port_name = var->get_id();

                    x << R"(
    // )" << top_module->get_id()
                      << "." << port_name << ":";

                    if (type.find("sc_bv<") == std::string::npos)
                    {
                        x << R"(
    if (auto d = )"
                          << "static_cast<uint64_t>(" << port_name << "_diff_"
                          << R"()))";
                    }
                    else // is a sc_bv<> type so we need to invoke its cast methods
                    {
                        x << R"(
    if (auto d = )"
                          << "static_cast<uint64_t>(" << port_name << "_diff_.to_uint64()"
                          << R"()))";
                    }
                    x << R"(
    {
        nz_triplet_list.push_back({ )"
                      << td_nmb << ", "
                      << "static_cast<uint16_t>(-1)"
                      << ", d });"
                      << R"(
    }
)";
                    ++td_nmb;
                }
            }
        }
    }

    x << R"(
    return nz_triplet_list;
}
)";

    x << R"(
)"
      << "std::vector<vrtlfi::td::UniqueElementTriplet> " << api_name << "Differential::gen_nz_triplet_vec(void) const"
      << R"(
{
    std::vector<vrtlfi::td::UniqueElementTriplet> nz_triplet_list{};
)";
    td_nmb = 0;
    core.foreach_module(write_triplet_push);

    if (core.is_systemc())
    {
        if (auto top_module = core.get_module_from_cell(core.get_top_cell()))
        {
            for (auto const &var : top_module->variables_)
            {
                std::string name = var->get_type();
                if (name == "in" || name == "out" || name == "inout")
                {

                    auto type = var->get_cxx_type();
                    auto port_name = var->get_id();

                    x << R"(
    // )" << top_module->get_id()
                      << "." << port_name << ":";

                    if (type.find("sc_bv<") == std::string::npos)
                    {
                        x << R"(
    if (auto d = )"
                          << "static_cast<uint64_t>(" << port_name << "_diff_"
                          << R"())
)";
                    }
                    else // is a sc_bv<> type so we need to invoke its cast methods
                    {
                        x << R"(
    if (auto d = )"
                          << "static_cast<uint64_t>(" << port_name << "_diff_.to_uint64()"
                          << R"())
)";
                    }
                    x << R"(
    {
        nz_triplet_list.push_back({ )"
                      << td_nmb << ", "
                      << "static_cast<uint16_t>(-1)"
                      << ", d });"
                      << R"(
    }
)";
                    ++td_nmb;
                }
            }
        }
    }

    x << R"(
    return nz_triplet_list;
}
)";

    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
