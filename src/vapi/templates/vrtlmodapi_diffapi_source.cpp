/*
 * Copyright 2024 Chair of EDA, Technical University of Munich
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

std::string VapiGenerator::VapiDiffSource::get_filename(void) const
{
    std::string top_name = gen_.get_core().get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4204
    // nothing to do here
#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif

    return util::concat(top_name, "_", API_DIFF_SOURCE_NAME);
}

std::string VapiGenerator::VapiDiffSource::generate_body(void) const
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
      << gen_.get_diffapiheader_filename() << R"("

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

    x << R"(
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

    bool hard_unroll = bool(DiffApiHardUnroll);

    auto write_triplet_push = [&](const types::Module &M) -> bool {
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
                    size_t element_idx = 0;
                    auto prefix_str = core.get_prefix(c, module_instance);
                    auto member_str = core.get_memberstr(c, t, prefix_str);

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
                        if (hard_unroll)
                        {
                            auto element_ctr = 0;
                            for (size_t k = 0; k < cxxdim[0]; ++k)
                            {
                                x << R"(
    if (auto d = )"
                                  << "static_cast<uint64_t>(" << xor_str << "[" << k << "]"
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
                        else
                        {
                            x << R"(
    for(size_t k = 0; k < )" << cxxdim[0]
                              << R"(/*K*/; ++k)
        if()" << xor_str << "[k] != 0)"
                              << R"(
            nz_triplet_list.push_back({ )"
                              << td_nmb << ", "
                              << "static_cast<uint16_t>(k)"
                              << ", " << util::concat("static_cast<uint64_t>(", xor_str, "[k]", ")") << R"(});
)";
                        }
                    }
                    break;
                    case 2:
                    {
                        if (hard_unroll)
                        {
                            auto element_ctr = 0;
                            for (size_t l = 0; l < cxxdim[0]; ++l)
                                for (size_t k = 0; k < cxxdim[1]; ++k)
                                {
                                    x << R"(
    if (auto d = )"
                                      << "static_cast<uint64_t>(" << xor_str << "[" << l << "]"
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
                        else
                        {
                            x << R"(
    for(size_t l = 0; l < )" << cxxdim[0]
                              << R"(/*L*/; ++l)
        for(size_t k = 0; k < )"
                              << cxxdim[1] << R"(/*K*/; ++k)
            if()" << xor_str << "[l][k] != 0)"
                              << R"(
                nz_triplet_list.push_back({ )"
                              << td_nmb << ", "
                              << util::concat("static_cast<uint16_t>(l * ", std::to_string(cxxdim[1]), "/*K*/ + k)")
                              << ", " << util::concat("static_cast<uint64_t>(", xor_str, "[l][k]", ")") << R"(});
)";
                        }
                    }
                    break;
                    case 3:
                    {
                        if (hard_unroll)
                        {
                            auto element_ctr = 0;
                            for (size_t m = 0; m < cxxdim[0]; ++m)
                                for (size_t l = 0; l < cxxdim[1]; ++l)
                                    for (size_t k = 0; k < cxxdim[2]; ++k)
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
                        else
                        {
                            x << R"(
    for(size_t m = 0; m < )" << cxxdim[0]
                              << R"(/*M*/; ++m)
        for(size_t l = 0; l < )"
                              << cxxdim[1] << R"(/*L*/; ++l)
            for(size_t k = 0; k < )"
                              << cxxdim[2] << R"(/*K*/; ++k)
                if()" << xor_str
                              << "[m][l][k] != 0)"
                              << R"(
                    nz_triplet_list.push_back({ )"
                              << td_nmb << ", "
                              << util::concat("static_cast<uint16_t>((m*", std::to_string(cxxdim[1]), "/*L*/ + l) * ",
                                              std::to_string(cxxdim[2]), "/*K*/ + k)")
                              << ", " << util::concat("static_cast<uint64_t>(", xor_str, "[m][l][k]", ")") << R"(});
)";
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
    {
        nz_triplet_list.push_back({ )"
                          << td_nmb << ", "
                          << "static_cast<uint16_t>(-1)"
                          << ", d });"
                          << R"(
    }
)";
                    }
                    else // is a sc_bv<> type so we go by word and set the element as subindex of the triplet
                    {
                        x << R"(
    for(size_t k = 0; k < )"
                          << port_name << R"(_diff_.size() ; ++k)
        if(__UNLIKELY()" << port_name
                          << "_diff_.get_word(k) != 0))"
                          << R"(
            nz_triplet_list.push_back({ )"
                          << td_nmb << ", "
                          << "static_cast<uint16_t>(k) "
                          << ", static_cast<uint64_t>(" << port_name << "_diff_.get_word(k)"
                          << ")});";
                    }

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
