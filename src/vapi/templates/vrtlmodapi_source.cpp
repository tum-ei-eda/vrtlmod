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
)";

    auto write_init_td = [&](const types::Module &M) -> bool {
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

                    std::string initializer_str = util::concat(
                        // clang-format off
                          "{ "
                        , util::concat("\"", prefix_str, ".", t.get_id(), "\"")
                        , ", "
                        , util::concat("vrtl_.", core.get_memberstr(c, t, prefix_str))
                        , ", "
                        , std::to_string(t.get_bits())
                        , ", "
                        , std::to_string(t.get_one_dim_bits())
                        , " }"
                        // clang-format on
                    );

                    std::string member_name = util::concat(prefix_str, "__DOT__", t.get_id(), "_");
                    util::strhelp::replaceAll(member_name, ".", "__DOT__");
                    util::strhelp::replaceAll(member_name, "->", "__REF__");
                    x << R"(
    , )" << member_name
                      << initializer_str;
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
                std::string name = var->get_type(); //< seems odd to get name by get_type, but the name is the XML type
                                                    // where id is the XML node type
                if (name == "in" || name == "out" || name == "inout")
                {
                    std::string decl_name = util::concat("TOP", "__DOT__", var->get_id(), "_");
                    util::strhelp::replaceAll(decl_name, ".", "__DOT__");
                    util::strhelp::replaceAll(decl_name, "->", "__REF__");
                    std::string initializer_str = util::concat(
                        // clang-format off
                          "{ "
                        , util::concat("/*name:*/", "\"", "TOP", ".", var->get_id(), "\"")
                        , ", "
                        , util::concat("/*data:*/", "vrtl_", ".", var->get_id())
                        , ", "
                        , util::concat("/*bits:*/", std::to_string(var->get_bits()))
                        , " }"
                        // clang-format on
                    );
                    x << R"(
    , )" << decl_name << initializer_str;
                }
            }
        }
    }
    x << R"(
)";

    x << R"(
)";

    std::string map_prefix = "";
    std::string retr_prefix = "";
    bool reverse_map = false;
    auto writetarget2idinitializer = [&](const types::Module &M) -> bool {
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
                    // auto member_str = util::concat("vrtl_.", core.get_memberstr(c, t, prefix_str));
                    // auto element_0 = member_str + "__td_";

                    std::string member_name = util::concat(prefix_str, "__DOT__", t.get_id(), "_");
                    util::strhelp::replaceAll(member_name, ".", "__DOT__");
                    util::strhelp::replaceAll(member_name, "->", "__REF__");
                    auto element_0 = util::concat("&", member_name);
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
                    std::string name = var->get_type(); //< seems odd to get name by get_type, but the name is the XML
                                                        // type where id is the XML node type
                    if (name == "in" || name == "out" || name == "inout")
                    {
                        std::string decl_name = util::concat("TOP", "__DOT__", var->get_id(), "_");
                        util::strhelp::replaceAll(decl_name, ".", "__DOT__");
                        util::strhelp::replaceAll(decl_name, "->", "__REF__");
                        // auto port_name = var->get_id();
                        auto element_0 = util::concat("&", decl_name); // util::concat("vrtl_.", port_name, "__td_");
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
                        x << "{ " << (reverse_map ? element_1 : element_0) << ", "
                          << (reverse_map ? element_0 : element_1) << " }";
                        ++td_nmb;
                    }
                }
            }
        }
    };

    retr_prefix = "";
    reverse_map = true;
    td_nmb = 0;
    x << R"(
    , )"
      << (reverse_map ? "id2target_{" : "target2id_{"); //(reverse_map ? "id2target_ = {" : "target2id_ = {");
    core.foreach_module(writetarget2idinitializer);
    write_systemc_target2id();
    x << R"(
    })";

    retr_prefix = "";
    reverse_map = false;
    td_nmb = 0;
    x << R"(
    , )"
      << (reverse_map ? "id2target_{" : "target2id_{"); //(reverse_map ? "id2target_ = {" : "target2id_ = {");
    core.foreach_module(writetarget2idinitializer);
    write_systemc_target2id();
    x << R"(
    })";
    x << R"(
{
    connect_vrtl2api();
}
)";

    auto write_connect_vrtl2api = [&](const types::Module &M) -> bool {
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
                    std::string vrtl_decl_name = util::concat("vrtl_.", core.get_memberstr(c, t, prefix_str));
                    std::string member_name = util::concat(prefix_str, "__DOT__", t.get_id(), "_");
                    util::strhelp::replaceAll(member_name, ".", "__DOT__");
                    util::strhelp::replaceAll(member_name, "->", "__REF__");
                    std::string map_key = util::concat(prefix_str, ".", t.get_id());
                    x << R"(
    )" << vrtl_decl_name
                      << "__td_"
                      << " = "
                      << "&" << member_name << ";";
                    x << R"(
    )"
                      << "td_[\"" << map_key << "\"] = "
                      << "&" << member_name << ";";
                }
            }
            return true;
        };
        core.foreach_injection_target(targetiterf);

        return true;
    };

    x << "void " << api_name << R"(::connect_vrtl2api(void)
{
)";
    td_nmb = 0;
    core.foreach_module(write_connect_vrtl2api);

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
                    std::string prefix_str = "TOP";
                    std::string member_name = util::concat(prefix_str, "__DOT__", var->get_id(), "_");
                    util::strhelp::replaceAll(member_name, ".", "__DOT__");
                    util::strhelp::replaceAll(member_name, "->", "__REF__");
                    std::string map_key = util::concat(prefix_str, ".", var->get_id());
                    x << R"(
    )"
                      << "td_[\"" << map_key << "\"] = "
                      << "&" << member_name << ";";
                    ++td_nmb;
                }
            }
        }
    }

    x << R"(
}

)";

    x << R"(void )" << api_name << R"(::dump_diff_csv(std::ostream& out) const
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
    for(size_t i = 0; i < vrtl_.)"
                          << port_name << ".read().length() ; ++i)"
                          << R"(
        out << vrtl_.)" << port_name
                          << R"(.read().get_bit(i) ? 1 : 0;
    out << std::endl;)";
                    }
                    else
                    {
                        x << R"(
    out << vrtl_.)" << port_name
                          << ".read() << std::endl;";
                    }
                }
            }
        }
    }
    x << R"(
}

)";

    x << R"(void )" << api_name << R"(::dump_diff_csv_vertical(std::ostream& out) const
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
    for(size_t i = 0; i < vrtl_.)"
                          << port_name << ".read().length() ; ++i)"
                          << R"(
        out << vrtl_.)" << port_name
                          << R"(.read().get_bit(i) ? 1 : 0;
    out << ",";)";
                    }
                    else
                    {
                        x << R"(
    out << vrtl_.)" << port_name
                          << R"(.read() << ",";)";
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
