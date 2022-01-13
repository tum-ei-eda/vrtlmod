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
/// @date Created on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vapi/generator.hpp"

namespace vapi
{

void VapiGenerator::VapiHeader::generate_body(void)
{
    std::stringstream x, entries;
    VapiGenerator &gen = VapiGenerator::_i();

    x << "#ifndef __" << gen.mTopTypeName << "VRTLMODAPI_VRTLMODAPI_HPP__ \n\
#define __"
      << gen.mTopTypeName << "VRTLMODAPI_VRTLMODAPI_HPP__ \n"
      << std::endl
      << "#include <vector> \n\
#include <memory> \n\
#include \"verilated.h\" \n\
#include \"verilated_heavy.h\" \n\n\
#include \""
      << gen.get_targetdictionary_relpath() << "\" \n"
      << std::endl
      << "class " << gen.mTopTypeName << ";" << std::endl
      << std::endl
      << std::endl
      << "class " << gen.mTopTypeName << "VRTLmodAPI : public vrtlfi::td::TD_API { \n\
public: \n\
	static "
      << gen.mTopTypeName << "VRTLmodAPI& i(void) { \n\
		static "
      << gen.mTopTypeName << "VRTLmodAPI _instance; \n\
		return (_instance); \n\
	} \n\
	private: \n\
	"
      << gen.mTopTypeName << "VRTLmodAPI(void); \n\
	"
      << gen.mTopTypeName << "VRTLmodAPI(" << gen.mTopTypeName << "VRTLmodAPI const&); \n\
	void operator=("
      << gen.mTopTypeName << "VRTLmodAPI const&); \n\
public: \n\
	std::shared_ptr<"
      << gen.mTopTypeName << "> vrtl_{nullptr}; \n\
	virtual ~"
      << gen.mTopTypeName << "VRTLmodAPI(void);\n\n";

    for (auto const &it : gen.mTargets)
    {
        if (it->mSeqInjCnt == 0)
            continue;

        x << "	std::shared_ptr< vrtlfi::td::";

        switch (it->mElData.cxxdim_.size())
        {
        case 0:
            x << "ZeroD_TDentry<decltype(\"" << it->get_hierarchyDedotted() << "\"_tstr)"
              << ", " << it->mElData.cxxbasetype_ << "> "
              << ">";
            break;
        case 1:
            x << "OneD_TDentry<decltype(\"" << it->get_hierarchyDedotted() << "\"_tstr)"
              << ", " << it->mElData.cxxbasetype_ << ", " << it->mElData.cxxtypedim_.back() << ", "
              << it->mElData.cxxdim_[0] << "> "
              << ">";
            break;
        case 2:
            x << "TwoD_TDentry<decltype(\"" << it->get_hierarchyDedotted() << "\"_tstr)"
              << ", " << it->mElData.cxxbasetype_ << ", " << it->mElData.cxxtypedim_.back() << ", "
              << it->mElData.cxxdim_[0] << ", " << it->mElData.cxxdim_[1] << "> "
              << ">";
            break;
        case 3:
            x << "ThreeD_TDentry<decltype(\"" << it->get_hierarchyDedotted() << "\"_tstr)"
              << ", " << it->mElData.cxxbasetype_ << ", " << it->mElData.cxxtypedim_.back() << ", "
              << it->mElData.cxxdim_[0] << ", " << it->mElData.cxxdim_[1] << ", " << it->mElData.cxxdim_[2] << "> "
              << ">";
            break;
        default:
            util::logging::log(util::logging::ERROR,
                               std::string("CType dimensions of injection target not supported: ") +
                                   it->mElData.vrtlCxxType);
            break;
        }
        x << " " << it->get_hierarchyDedotted() << "_{};\n";
        //"	std::shared_ptr<TDentry> " << it->get_hierarchyDedotted() << "_{};\n";
    }

    x << std::endl
      << "}; \n"
         "#endif /* __"
      << gen.mTopTypeName << "VRTLMODAPI_VRTLMODAPI_HPP__ */";

    body_ = x.str();
}

} // namespace vapi
