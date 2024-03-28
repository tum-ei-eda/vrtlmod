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

std::string VapiGenerator::VapiDiffCompareSource::get_filename(void) const
{
    std::string top_name = gen_.get_core().get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4204
    // nothing to do here
#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif

    return util::concat(top_name, "_", API_DIFF_COMPARE_SOURCE_NAME);
}

std::string VapiGenerator::VapiDiffCompareSource::generate_body(void) const
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
    bool hard_unroll = bool(DiffApiHardUnroll); // if false we de-serialize the element index for multi-dimensional accesses

    auto writediffbody = [&](const types::Module &M) -> bool {
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
                        for (size_t k = 0; k < cxxdim[0]; ++k)
                        {
                            x << R"(
                case )" << element_idx
                              << ":"
                              << R"(
                {
                    auto d = ()"
                              << xor_str << "[" << k << "]"
                              << " ^ "
                              << "val"
                              << ") & 0x" << std::hex << t.get_element_mask({ k }) << std::dec << ";";
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
                        for (size_t l = 0; l < cxxdim[0]; ++l)
                            for (size_t k = 0; k < cxxdim[1]; ++k)
                            {
                                x << R"(
                case )" << element_idx
                                  << ":"
                                  << R"(
                {
                    auto d = ()" << xor_str
                                  << "[" << l << "]"
                                  << "[" << k << "]"
                                  << " ^ "
                                  << "val"
                                  << ") & 0x" << std::hex << t.get_element_mask({ l, k }) << std::dec << ";";
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
                        for (size_t m = 0; m < cxxdim[0]; ++m)
                            for (size_t l = 0; l < cxxdim[1]; ++l)
                                for (size_t k = 0; k < cxxdim[2]; ++k)
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
            auto d = (static_cast<uint64_t>()" << port_name
                          << "_diff_.get_word(element_idx))"
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

    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
