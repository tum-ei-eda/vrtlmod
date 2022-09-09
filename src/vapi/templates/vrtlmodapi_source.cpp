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
    const auto &core = gen_.get_core();
    std::string top_type = core.get_top_cell().get_type();
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

    auto celliterf = [&](const types::Cell &c) -> bool
    {
        if (const types::Module *m = core.get_module_from_cell(c))
        {
            auto targetiterf = [&](const types::Target &t) -> bool
            {
                if (t.get_parent() == *m)
                {
                    for (const auto &module_instance : t.get_parent().symboltable_instances_)
                    {
                        std::stringstream smart_type, initializer;
                        std::string type_str, initializer_str;
                        std::string prefix_str =
                            (c == core.get_top_cell()) ? core.get_top_cell().get_id() : module_instance;
                        std::string member_str = util::concat("vrtl_.__VlSymsp->", prefix_str,
                                                              (c == core.get_top_cell()) ? "->" : ".", t.get_id());
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
        }
        else
        {
            LOG_FATAL("API GEN: ", "Can not find Module for Cell ", c.get_id(), "[", c.get_type(), "]");
        }
        return true;
    };
    core.foreach_cell(celliterf);

    x << "}" << std::endl;
    x << top_type << "VRTLmodAPI::~" << top_type << "VRTLmodAPI(void) { \n\
} \n" << std::endl;

    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
