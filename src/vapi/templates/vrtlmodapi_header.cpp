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
/// @file vrtlmodapi_header.cpp
/// @date Created on Wed Dec 09 13:32:12 2020
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vapi/generator.hpp"
#include "vrtlmod/core/core.hpp"
#include "vrtlmod/core/types.hpp"

namespace vrtlmod
{
namespace vapi
{

std::string VapiGenerator::VapiHeader::generate_body(void) const
{
    const auto &core = gen_.get_core();
    std::string top_type = core.get_top_cell().get_type();
    std::stringstream x, entries;

    x << "#ifndef __" << top_type << "VRTLMODAPI_VRTLMODAPI_HPP__ \n\
#define __"
      << top_type << "VRTLMODAPI_VRTLMODAPI_HPP__ \n"
      << std::endl
      << "#include <vector> \n\
#include <memory> \n\
#include \"verilated.h\" \n\
#include \"verilated_heavy.h\" \n\n\
#include \""
      << gen_.get_targetdictionary_relpath() << "\" \n"
      << std::endl
      << "class " << top_type << ";" << std::endl
      << std::endl
      << std::endl
      << "class " << top_type << "VRTLmodAPI : public vrtlfi::td::TD_API { \n\
  public: \n\
    static "
      << top_type << "VRTLmodAPI& i(void) { \n\
        static "
      << top_type << "VRTLmodAPI _instance; \n\
        return (_instance); \n\
    } \n\
  private: \n\
    " << top_type
      << "VRTLmodAPI(void); \n\
	"
      << top_type << "VRTLmodAPI(" << top_type << "VRTLmodAPI const&); \n\
    void operator=("
      << top_type << "VRTLmodAPI const&); \n\
  public: \n\
    std::shared_ptr<"
      << top_type << "> vrtl_{nullptr}; \n\
    virtual ~"
      << top_type << "VRTLmodAPI(void);\n\n";

    auto func = [&](const types::Target &t)
    {
        if (t.get_seq_assignment_count() != 0)
        {
            x << "    std::shared_ptr< vrtlfi::td::";

            auto cxxdim = t.get_cxx_dimension_lengths();
            auto cxxdimtypes = t.get_cxx_dimension_types();

            switch (cxxdim.size())
            {
            case 0:
                x << "ZeroD_TDentry<" << t.get_cxx_type() << "> ";
                break;
            case 1:
                x << "OneD_TDentry<" << t.get_cxx_type() << ", " << cxxdimtypes.back() << ", " << cxxdim[0] << "> ";
                break;
            case 2:
                x << "TwoD_TDentry<" << t.get_cxx_type() << ", " << cxxdimtypes.back() << ", " << cxxdim[0] << ", "
                  << cxxdim[1] << "> ";
                break;
            case 3:
                x << "ThreeD_TDentry<" << t.get_cxx_type() << ", " << cxxdimtypes.back() << ", " << cxxdim[0] << ", "
                  << cxxdim[1] << ", " << cxxdim[2] << "> ";
                break;
            default:
                LOG_ERROR("CType dimensions of injection target not supported: ", t.get_cxx_type());
                break;
            }
            x << ">";
            x << " " << t.get_id() << "_{};\n";
        }
        return true;
    };
    core.foreach_injectable(func);

    x << std::endl
      << "}; \n"
         "#endif /* __"
      << top_type << "VRTLMODAPI_VRTLMODAPI_HPP__ */";

    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
