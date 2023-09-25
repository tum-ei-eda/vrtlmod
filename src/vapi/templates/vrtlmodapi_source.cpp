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
#if VRTLMOD_VERILATOR_VERSION <= 4204
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
#if VRTLMOD_VERILATOR_VERSION <= 4204
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
//#if VRTLMOD_VERILATOR_VERSION <= 4204
//#else // VRTLMOD_VERILATOR_VERSION <= 4228
//        return (*c == core.get_top_cell()) ? "rootp" : module_instance;
//#endif
        return (*c == core.get_top_cell()) ? core.get_top_cell().get_id() : module_instance;
    };

    auto get_memberstr = [&](types::Cell const *c, const types::Target &t, const std::string &prefix) -> std::string
    {
        return util::concat(
#if VRTLMOD_VERILATOR_VERSION <= 4204
            SYMBOLTABLE_NAME, "->", prefix, (*c == core.get_top_cell()) ? "->" : "."
#else // VRTLMOD_VERILATOR_VERSION <= 4228
            "rootp->", SYMBOLTABLE_NAME, "->", prefix, "."
#endif
            , t.get_id());
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

    x << "}" << std::endl;
    x << top_type << "VRTLmodAPI::~" << top_type << "VRTLmodAPI(void) { \n\
}\n" << std::endl;

    x << api_name << "Differential::" << api_name << "Differential(const " << api_name << "& faulty, const " << api_name
      << R"(& reference)
    : )"
      << api_name << "(\"Differential\")"
      << R"(
    , faulty_(faulty)
    , reference_(reference)
{)";

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
    faulty_target2id_[faulty_.get_target)"
                      << "(\"" << prefix_str << "." << t.get_id() << "\")] = " << td_nmb << R"(;
    reference_target2id_[reference_.get_target)"
                      << "(\"" << prefix_str << "." << t.get_id() << "\")] = " << td_nmb << R"(;
    diff_target2id_[get_target)"
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

    td_nmb = 0;
    core.foreach_module(writetarget2idinit);
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
                    x << "// )" << prefix_str << "." << t.get_id() << ":";

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
                                        ret += )"
                                  << xor_str << "[" << m << "] ? 1 : 0;";
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
                                      << "[" << l << "]) & 0x" << std::hex << t.get_element_mask({ l, m }) << std::dec
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
                                      << "[" << l << "]) & 0x" << std::hex << t.get_element_mask({ l, m }) << std::dec
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
                                          << "[" << k << "]) & 0x" << std::hex << t.get_element_mask({ k, l, m })
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
                                          << "[" << k << "]) & 0x" << std::hex << t.get_element_mask({ k, l, m })
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
                    if (!util::strhelp::replace(type, "sc_out<", "sc_signal<"))
                        if (!util::strhelp::replace(type, "sc_in<", "sc_signal<"))
                            util::strhelp::replace(type, "sc_inout<", "sc_signal<");

                    x << R"(
    )" << port_name << "_diff_ = faulty_.vrtl_."
                      << port_name << ".read() ^ reference_.vrtl_." << port_name << ".read();"
                      << R"(
    )";
                    if (type.find("sc_bv<") != std::string::npos)
                    {
                        x << "for(size_t i = 0; i < " << port_name << "_diff_.read().size() ; ++i)"
                          << R"(
        ret += )" << port_name
                          << "_diff_.read().get_word(i) ? 1 : 0;";
                    }
                    else
                    {
                        x << "ret += " << port_name << "_diff_.read() ? 1 : 0;";
                    }
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
    if(auto pos = faulty_target2id_.find(target); pos != faulty_target2id_.end())
    {
        return pos->second;
    }
    if(auto pos = reference_target2id_.find(target); pos != reference_target2id_.end())
    {
        return pos->second;
    }
    if(auto pos = diff_target2id_.find(target); pos != diff_target2id_.end())
    {
        return pos->second;
    }
    return 0;
}

)";

    x << R"(void )" << api_name << R"(Differential::dump_diff_csv(std::ostream& out) const
{
    for(auto const& it: diff_target2id_)
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
                    if (!util::strhelp::replace(type, "sc_out<", "sc_signal<"))
                        if (!util::strhelp::replace(type, "sc_in<", "sc_signal<"))
                            util::strhelp::replace(type, "sc_inout<", "sc_signal<");
                    x << R"(
    out << ")" << port_name
                      << "[" << name << "], 0b\";";
                    if (type.find("sc_bv<") != std::string::npos)
                    {
                        x << R"(
    for(size_t i = 0; i < )"
                          << port_name << "_diff_.read().length() ; ++i)"
                          << R"(
        out << )" << port_name
                          << R"(_diff_.read().get_bit(i) ? 1 : 0;
    out << std::endl;)";
                    }
                    else
                    {
                        x << R"(
    out << )" << port_name << "_diff_.read() << std::endl;";
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
        for(auto it = diff_target2id_.begin(); it != diff_target2id_.end(); ++it)
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
                    if (!util::strhelp::replace(type, "sc_out<", "sc_signal<"))
                        if (!util::strhelp::replace(type, "sc_in<", "sc_signal<"))
                            util::strhelp::replace(type, "sc_inout<", "sc_signal<");
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

    for(auto it = diff_target2id_.begin(); it != diff_target2id_.end(); ++it)
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
                    if (!util::strhelp::replace(type, "sc_out<", "sc_signal<"))
                        if (!util::strhelp::replace(type, "sc_in<", "sc_signal<"))
                            util::strhelp::replace(type, "sc_inout<", "sc_signal<");
                    x << R"(
    out << "0b";)";
                    if (type.find("sc_bv<") != std::string::npos)
                    {
                        x << R"(
    for(size_t i = 0; i < )"
                          << port_name << "_diff_.read().length() ; ++i)"
                          << R"(
        out << )" << port_name
                          << R"(_diff_.read().get_bit(i) ? 1 : 0;
    out << ",";)";
                    }
                    else
                    {
                        x << R"(
    out << )" << port_name << R"(_diff_.read() << ",";)";
                    }
                }
            }
        }
    }
    x << R"(
    out << std::endl;
}
)";

    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
