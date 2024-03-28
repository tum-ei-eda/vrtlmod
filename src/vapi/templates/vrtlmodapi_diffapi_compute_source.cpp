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

std::string VapiGenerator::VapiDiffComputeSource::get_filename(void) const
{
    std::string top_name = gen_.get_core().get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4204
    // nothing to do here
#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif

    return util::concat(top_name, "_", API_DIFF_COMPUTE_SOURCE_NAME);
}

std::string VapiGenerator::VapiDiffComputeSource::generate_body(void) const
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
    bool hard_unroll =
        bool(DiffApiHardUnroll); // one statement for basic Ctype element XOR for complex data types a[M][L][K] -> unroll M-L-K
    bool hard_mask = false; // after XOR bit-wise AND with bit-vector to ensure Ctypes are aliased to RTL bits

    auto writecomparebody = [&](const types::Module &M) -> bool {
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
    )";

                    x << "// " << prefix_str << "." << t.get_id() << ":";

                    switch (cxxdim.size())
                    {
                    case 0:
                    {

                        x << R"(
    )" << xor_str << " = ("
                          << lhs_str << " ^ " << rhs_str << ") & 0x" << std::hex << t.get_element_mask({}) << std::dec
                          << ";";
                        x << R"(
    ret += )" << xor_str << "? 1 : 0;";
                    }
                    break;
                    case 1:
                    {
                        if (hard_unroll)
                        {
                            for (size_t k = 0; k < cxxdim[0]; ++k)
                            {
                                x << R"(
    )" << xor_str << "[" << k << "]"
                                  << " = (" << lhs_str << "[" << k << "]"
                                  << " ^ " << rhs_str << "[" << k << "])"
                                  << " & 0x" << std::hex << t.get_element_mask({ k }) << std::dec << ";";
                                x << R"(
    ret += )" << xor_str << "[" << k
                                  << "] ? 1 : 0;";
                            }
                        }
                        else
                        {
                            x << R"(
    for(size_t k = 0; k < )" << cxxdim[0]
                              << R"(/*K*/; ++k)
    {
        )" << xor_str << "[k]"
                              << " = (" << lhs_str << "[k]"
                              << " ^ " << rhs_str << "[k]);"
                              << R"(
        if(__UNLIKELY()" << xor_str
                              << R"([k] != 0))
            ++ret;
    }
)";
                        }
                    }
                    break;
                    case 2:
                    {
                        if (hard_unroll)
                        {
                            for (size_t l = 0; l < cxxdim[0]; ++l)
                                for (size_t k = 0; k < cxxdim[1]; ++k)
                                {
                                    x << R"(
    )" << xor_str << "[" << l << "]"
                                      << "[" << k << "]"
                                      << " = (" << lhs_str << "[" << l << "]"
                                      << "[" << k << "]"
                                      << " ^ " << rhs_str << "[" << l << "]"
                                      << "[" << k << "]) & 0x" << std::hex << t.get_element_mask({ l, k }) << std::dec
                                      << ";";
                                    x << R"(
    ret += )" << xor_str << "[" << l << "]"
                                      << "[" << k << "] ? 1 : 0;";
                                }
                        }
                        else
                        {
                            x << R"(
    for(size_t l = 0; l < )" << cxxdim[0]
                              << "/*L*/; ++l)"
                              << R"(
        for(size_t k = 0; k < )"
                              << cxxdim[1] << "/*K*/; ++k)"
                              << R"(
        {
            )" << xor_str << "[l][k]"
                              << " = (" << lhs_str << "[l][k]"
                              << " ^ " << rhs_str << "[l][k]);"
                              << R"(
            if(__UNLIKELY()" << xor_str
                              << R"([l][k] != 0))
                ++ret;
        }
)";
                        }
                    }
                    break;
                    case 3:
                    {
                        if (hard_unroll)
                        {
                            for (size_t m = 0; m < cxxdim[0]; ++m)
                                for (size_t l = 0; l < cxxdim[1]; ++l)
                                    for (size_t k = 0; k < cxxdim[2]; ++k)
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
                        }
                        else
                        {
                            x << R"(
    for(size_t m = 0; m < )" << cxxdim[0]
                              << "/*M*/; ++m)"
                              << R"(
        for(size_t l = 0; l < )"
                              << cxxdim[1] << "/*L*/; ++l)"
                              << R"(
            for(size_t k = 0; k < )"
                              << cxxdim[2] << "/*K*/; ++k)"
                              << R"(
            {
                )" << xor_str << "[m][l][k]"
                              << " = (" << lhs_str << "[m][l][k]"
                              << " ^ " << rhs_str << "[m][l][k]);"
                              << R"(
                if(__UNLIKELY()"
                              << xor_str << R"([m][l][k] != 0))
                    ++ret;
            }
)";
                        }
                    }
                    break;
                    default:
                        LOG_ERROR("CType dimensions of injection target not supported: ", t.get_cxx_type());
                        break;
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

                    if (type.find("sc_bv<") != std::string::npos)
                    {
                        x << R"(
    for(size_t k = 0; k < )"
                          << port_name << "_diff_.size() ; ++k)"
                          << R"(
    {
        auto d = faulty_.vrtl_.)"
                          << port_name << ".read().get_word(k) ^ reference_.vrtl_." << port_name
                          << ".read().get_word(k);"
                          << R"(
        )" << port_name << "_diff_.set_word(k, d);"
                          << R"(
        ret += )" << port_name
                          << "_diff_.get_word(k) ? 1 : 0;"
                          << R"(
    }
)";
                    }
                    else
                    {
                        x << R"(
    )" << port_name << "_diff_ = faulty_.vrtl_."
                          << port_name << ".read() ^ reference_.vrtl_." << port_name << ".read();"
                          << R"(
    )";
                        x << "ret += " << port_name << "_diff_ ? 1 : 0;"
                          << R"(
)";
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

    auto writecompute_diff_vector = [&](const types::Module &M) -> bool {
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
    )";

                    x << "// " << prefix_str << "." << t.get_id() << ":";

                    switch (cxxdim.size())
                    {
                    case 0:
                    {

                        x << R"(
    )" << xor_str << " = ("
                          << lhs_str << " ^ " << rhs_str << ")";
                        if (hard_mask)
                        {
                            x << " & 0x" << std::hex << t.get_element_mask({}) << std::dec << ";";
                        }
                        x << R"(;
    if(__UNLIKELY()" << xor_str
                          << "!= 0))"
                          << R"(
        diff_vec.push_back({ )"
                          << td_nmb << ", "
                          << "static_cast<uint16_t>(-1)"
                          << ", static_cast<uint64_t>(" << xor_str << ")});"
                          << R"(
)";
                    }
                    break;
                    case 1:
                    {
                        if (hard_unroll)
                        {
                            size_t element_ctr = 0;
                            for (size_t k = 0; k < cxxdim[0]; ++k)
                            {
                                x << R"(
    )" << xor_str << "[" << k << "]"
                                  << " = (" << lhs_str << "[" << k << "]"
                                  << " ^ " << rhs_str << "[" << k << "])";
                                if (hard_mask)
                                {
                                    x << " & 0x" << std::hex << t.get_element_mask({ k }) << std::dec;
                                }
                                x << R"(;
    if(__UNLIKELY()" << xor_str << "["
                                  << k << "]"
                                  << " != 0))"
                                  << R"(
        diff_vec.push_back({ )" << td_nmb
                                  << ", " << element_ctr << ", static_cast<uint64_t>(" << xor_str << "[" << k << "]"
                                  << ")});"
                                  << R"(
)";
                                ++element_ctr;
                            }
                        }
                        else
                        {
                            x << R"(
    for(size_t k = 0; k < )" << cxxdim[0]
                              << R"(/*K*/; ++k)
    {
        )" << xor_str << "[k]"
                              << " = (" << lhs_str << "[k]"
                              << " ^ " << rhs_str << "[k]);"
                              << R"(
        if(__UNLIKELY()" << xor_str
                              << "[k]"
                              << " != 0))"
                              << R"(
        diff_vec.push_back({ )"
                              << td_nmb << ", "
                              << "static_cast<uint16_t>(k)"
                              << ", static_cast<uint64_t>(" << xor_str << "[k]"
                              << ")});"
                              << R"(
    }
)";
                        }
                    }
                    break;
                    case 2:
                    {
                        if (hard_unroll)
                        {
                            size_t element_ctr = 0;
                            for (size_t l = 0; l < cxxdim[0]; ++l)
                                for (size_t k = 0; k < cxxdim[1]; ++k)
                                {
                                    x << R"(
    )" << xor_str << "[" << l << "]"
                                      << "[" << k << "]"
                                      << " = (" << lhs_str << "[" << l << "]"
                                      << "[" << k << "]"
                                      << " ^ " << rhs_str << "[" << l << "]"
                                      << "[" << k << "])";
                                    if (hard_mask)
                                    {
                                        x << " & 0x" << std::hex << t.get_element_mask({ l, k }) << std::dec << ";";
                                    }
                                    x << R"(;
    if(__UNLIKELY()" << xor_str << "[" << l
                                      << "]"
                                      << "[" << k << "]"
                                      << " != 0))"
                                      << R"(
        diff_vec.push_back({ )" << td_nmb
                                      << ", " << element_ctr << ", static_cast<uint64_t>(" << xor_str << "[" << l << "]"
                                      << "[" << k << "]"
                                      << ")});"
                                      << R"(
)";
                                    ++element_ctr;
                                }
                        }
                        else
                        {
                            x << R"(
    for(size_t l = 0; l < )" << cxxdim[0]
                              << "/*L*/; ++l)"
                              << R"(
        for(size_t k = 0; k < )"
                              << cxxdim[1] << "/*K*/; ++k)"
                              << R"(
        {
            )" << xor_str << "[l][k]"
                              << " = (" << lhs_str << "[l][k]"
                              << " ^ " << rhs_str << "[l][k]);"
                              << R"(
            if(__UNLIKELY()" << xor_str
                              << "[l][k]"
                              << " != 0))"
                              << R"(
                diff_vec.push_back({ )"
                              << td_nmb << ", "
                              << "static_cast<uint16_t>(l * (" << cxxdim[1] << "/*K*/) + k)"
                              << ", static_cast<uint64_t>(" << xor_str << "[l][k]"
                              << ")});"
                              << R"(
        }
)";
                        }
                    }
                    break;
                    case 3:
                    {
                        if (hard_unroll)
                        {
                            size_t element_ctr = 0;
                            for (size_t m = 0; m < cxxdim[0]; ++m)
                                for (size_t l = 0; l < cxxdim[1]; ++l)
                                    for (size_t k = 0; k < cxxdim[2]; ++k)
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
                                          << "[" << k << "])";
                                        if (hard_mask)
                                        {
                                            x << " & 0x" << std::hex << t.get_element_mask({ m, l, k }) << std::dec
                                              << ";";
                                        }
                                        x << R"(;
    if(__UNLIKELY()" << xor_str << "[" << m
                                          << "]"
                                          << "[" << l << "]"
                                          << "[" << k << "]"
                                          << " != 0))"
                                          << R"(
        diff_vec.push_back({ )" << td_nmb << ", "
                                          << element_ctr << ", static_cast<uint64_t>(" << xor_str << "[" << m << "]"
                                          << "[" << l << "]"
                                          << "[" << k << "]"
                                          << ")});"
                                          << R"(
)";
                                        ++element_ctr;
                                    }
                        }
                        else
                        {
                            x << R"(
    for(size_t m = 0; m < )" << cxxdim[0]
                              << "/*M*/; ++m)"
                              << R"(
        for(size_t l = 0; l < )"
                              << cxxdim[1] << "/*L*/; ++l)"
                              << R"(
            for(size_t k = 0; k < )"
                              << cxxdim[2] << "/*K*/; ++k)"
                              << R"(
            {
                )" << xor_str << "[m][l][k]"
                              << " = (" << lhs_str << "[m][l][k]"
                              << " ^ " << rhs_str << "[m][l][k]);"
                              << R"(
                if(__UNLIKELY()"
                              << xor_str << "[m]"
                              << "[l]"
                              << "[k]"
                              << " != 0))"
                              << R"(
                    diff_vec.push_back({ )"
                              << td_nmb << ", "
                              << "static_cast<uint16_t>(m * (" << cxxdim[1] << "/*L*/ * " << cxxdim[2] << "/*K*/) + (l*"
                              << cxxdim[2] << "/*K*/) + k)"
                              << ", static_cast<uint64_t>(" << xor_str << "[m][l][k]"
                              << ")});"
                              << R"(
            }
)";
                        }
                    }
                    break;
                    default:
                        LOG_ERROR("CType dimensions of injection target not supported: ", t.get_cxx_type());
                        break;
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
      << "std::vector<vrtlfi::td::UniqueElementTriplet> " << api_name << "Differential::compute_diff_vector(void)"
      << R"(
{
    std::vector<vrtlfi::td::UniqueElementTriplet> diff_vec{};
)";

    td_nmb = 0;
    core.foreach_module(writecompute_diff_vector);

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
                    if (type.find("sc_bv<") != std::string::npos)
                    {
                        x << R"(
    for(size_t k = 0; k < )"
                          << port_name << R"(_diff_.size() ; ++k)
    {
        auto d = faulty_.vrtl_.)"
                          << port_name << ".read().get_word(k) ^ reference_.vrtl_." << port_name
                          << ".read().get_word(k);"
                          << R"(
        )" << port_name << "_diff_.set_word(k, d);"
                          << R"(
        if(__UNLIKELY()" << port_name
                          << "_diff_.get_word(k) != 0))"
                          << R"(
            diff_vec.push_back({ )"
                          << td_nmb << ", "
                          << "static_cast<uint16_t>(k) "
                          << ", static_cast<uint64_t>(" << port_name << "_diff_.get_word(k)"
                          << R"() });
    }
)";
                    }
                    else
                    {
                        x << R"(
    )" << port_name << "_diff_ = faulty_.vrtl_."
                          << port_name << ".read() ^ reference_.vrtl_." << port_name << ".read();";
                        x << R"(
    if(__UNLIKELY()" << port_name
                          << "_diff_ != 0))"
                          << R"(
        diff_vec.push_back({ )"
                          << td_nmb << ", "
                          << "static_cast<uint16_t>(-1) "
                          << ", static_cast<uint64_t>(" << port_name << "_diff_"
                          << ")});";
                    }
                    ++td_nmb;
                }
            }
        }
    }

    x << R"(
    return diff_vec;
}
)";

    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
