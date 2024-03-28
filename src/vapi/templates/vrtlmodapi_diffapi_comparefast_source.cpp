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
#include "vrtlmod/util/logging.hpp"

#include <boost/algorithm/string/replace.hpp>

#include "llvm/Support/CommandLine.h"
extern llvm::cl::opt<bool> DiffApiHardUnroll;

namespace vrtlmod
{
namespace vapi
{

std::string VapiGenerator::VapiDiffCompareFastSource::get_filename(void) const
{
    std::string top_name = gen_.get_core().get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4204
    // nothing to do here
#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif

    return util::concat(top_name, "_", API_DIFF_COMPARE_FAST_SOURCE_NAME);
}

std::string VapiGenerator::VapiDiffCompareFastSource::generate_body(void) const
{
    int td_nmb = 0;

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
      << gen_.get_diffapiheader_filename() << R"("

)";
    bool hard_unroll = bool(DiffApiHardUnroll);

    auto writecomparebody_for_module = [&](const types::Module &M) -> bool {
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
                    auto member_str = core.get_memberstr(c, t, prefix_str);

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
                        if (hard_unroll)
                        {
                            x << R"(
            if(__UNLIKELY(()" << lhs_str
                              << " ^ " << rhs_str << ") & 0x" << std::hex << t.get_element_mask({}) << std::dec << "))"
                              << R"(
                return faulty_.td_.at)"
                              << "(\"" << prefix_str << "." << t.get_id() << "\").get();";
                        }
                        else
                        {
                            x << R"(
            if(__UNLIKELY()" << lhs_str
                              << " ^ " << rhs_str << "))"
                              << R"(
                return )" << lhs_str
                              << "__td_;";
                        }
                    }
                    break;
                    case 1:
                    {
                        if (hard_unroll)
                        {
                            for (size_t m = 0; m < cxxdim[0]; ++m)
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
                        else // stick with loops.
                        {
                            x << R"(
            for(size_t m = 0; m < )"
                              << cxxdim[0] << R"(; ++m)
                if(__UNLIKELY()"
                              << lhs_str << "[m]"
                              << " ^ " << rhs_str << "[m]"
                              << "))"
                              << R"(
                    return )" << lhs_str
                              << "__td_;";
                        }
                    }
                    break;
                    case 2:
                    {
                        if (hard_unroll)
                        {
                            for (size_t l = 0; l < cxxdim[1]; ++l)
                                for (size_t m = 0; m < cxxdim[0]; ++m)
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
                        else
                        {
                            x << R"(
            for(size_t m = 0; m < )"
                              << cxxdim[0] << R"(; ++m)
                for(size_t l = 0; l < )"
                              << cxxdim[1] << R"(; ++l)
                    if(__UNLIKELY()"
                              << lhs_str << "[m][l]"
                              << " ^ " << rhs_str << "[m][l]"
                              << "))"
                              << R"(
                        return )"
                              << lhs_str << "__td_;";
                        }
                    }
                    break;
                    case 3:
                    {
                        if (hard_unroll)
                        {
                            for (size_t k = 0; k < cxxdim[2]; ++k)
                                for (size_t l = 0; l < cxxdim[1]; ++l)
                                    for (size_t m = 0; m < cxxdim[0]; ++m)
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
                        else
                        {
                            x << R"(
            for(size_t m = 0; m < )"
                              << cxxdim[0] << R"(; ++m)
                for(size_t l = 0; l < )"
                              << cxxdim[1] << R"(; ++l)
                    for(size_t k = 0; k < )"
                              << cxxdim[2] << R"(; ++k)
                        if(__UNLIKELY()"
                              << lhs_str << "[m][l][k]"
                              << " ^ " << rhs_str << "[m][l][k]"
                              << "))"
                              << R"(
                            return )"
                              << lhs_str << "__td_;";
                        }
                    }
                    break;
                    default:
                        LOG_ERROR("CType dimensions of injection target not supported: ", t.get_cxx_type());
                        break;
                    }
                    x << R"(
        [[ fallthrough ]];)";

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
      << "vrtlfi::td::TDentry const* " << api_name
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
    core.foreach_module(writecomparebody_for_module);

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
                    
                    auto type = var->get_cxx_type();
                    auto port_name = var->get_id();
                    std::string prefix_str = "TOP";
                    std::string member_name = util::concat(prefix_str, "__DOT__", var->get_id(), "_");
                    util::strhelp::replaceAll(member_name, ".", "__DOT__");
                    util::strhelp::replaceAll(member_name, "->", "__REF__");
                    auto diff_expr =
                        util::concat("faulty_.vrtl_.", port_name, ".read() ^ reference_.vrtl_.", port_name, ".read()");

                    x << R"(
        case )" << td_nmb
                      << ": ";
                    x << R"(
            if(__UNLIKELY(()"
                      << diff_expr << ") != 0))"
                      << R"(
                return &(faulty_.)" << member_name << ");";
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

    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
