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
/// @date Created on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vapi/generator.hpp"
#include "vrtlmod/util/logging.hpp"

#include <boost/algorithm/string/replace.hpp>

namespace vapi
{

void VapiGenerator::VapiSource::generate_body(void)
{
    std::stringstream x, entries;
    VapiGenerator &gen = VapiGenerator::_i();

    x << "// Vrtl-specific includes: \n\
#include \""
      << gen.get_vrtltopheader_filename() << "\" \n\
#include \""
      << gen.get_vrtltopsymsheader_filename() << "\" \n\
// General API includes: \n\
#include <memory> \n\
#include <iostream> \n\
#include \"verilated.h\" \n\
#include \""
      << gen.get_targetdictionary_relpath() << "\" \n\
#include \""
      << gen.get_apiheader_filename() << "\" \n"
      << std::endl
      << "" << gen.mTopTypeName << "VRTLmodAPI::" << gen.mTopTypeName << "VRTLmodAPI(void) \n\
	: vrtlfi::td::TD_API() \n\
{\n\
	vrtl_ = std::make_shared<"
      << gen.mTopTypeName << ">(";
    if (gen.is_systemc())
    {
        x << "\"" << gen.mTopTypeName << "\"";
    }
    x << "); \n" << std::endl;
    int i = 0;
    for (auto const &it : gen.mTargets)
    {
        if (it->mSeqInjCnt == 0)
            continue;
        x << "	" << it->get_hierarchyDedotted() << "_ = std::make_shared< vrtlfi::td::";
        switch (it->mElData.cxxdim_.size())
        {
        case 0:
            x << "ZeroD_TDentry<decltype(\"" << it->get_hierarchyDedotted() << "\"_tstr)"
              << ", " << it->mElData.cxxbasetype_ << "> "
              << ">("
              << "vrtl_->__VlSymsp->TOPp->" << boost::replace_all_copy(it->get_hierarchy(), ".", "->") << ", " << i
              << ", " << it->mElData.nmbBits << ", " << it->mElData.nmbonedimBits << ");" << std::endl;
            break;
        case 1:
            x << "OneD_TDentry<decltype(\"" << it->get_hierarchyDedotted() << "\"_tstr)"
              << ", " << it->mElData.cxxbasetype_ << ", " << it->mElData.cxxtypedim_.back() << ", "
              << it->mElData.cxxdim_[0] << "> "
              << ">("
              << "vrtl_->__VlSymsp->TOPp->" << boost::replace_all_copy(it->get_hierarchy(), ".", "->") << ", " << i
              << ", " << it->mElData.nmbBits << ", " << it->mElData.nmbonedimBits << ");" << std::endl;
            break;
        case 2:
            x << "TwoD_TDentry<decltype(\"" << it->get_hierarchyDedotted() << "\"_tstr)"
              << ", " << it->mElData.cxxbasetype_ << ", " << it->mElData.cxxtypedim_.back() << ", "
              << it->mElData.cxxdim_[0] << ", " << it->mElData.cxxdim_[1] << "> "
              << ">("
              << "vrtl_->__VlSymsp->TOPp->" << boost::replace_all_copy(it->get_hierarchy(), ".", "->") << ", " << i
              << ", " << it->mElData.nmbBits << ", " << it->mElData.nmbonedimBits << ");" << std::endl;
            break;
        case 3:
            x << "ThreeD_TDentry<decltype(\"" << it->get_hierarchyDedotted() << "\"_tstr)"
              << ", " << it->mElData.cxxbasetype_ << ", " << it->mElData.cxxtypedim_.back() << ", "
              << it->mElData.cxxdim_[0] << ", " << it->mElData.cxxdim_[1] << ", " << it->mElData.cxxdim_[2] << "> "
              << ">("
              << "vrtl_->__VlSymsp->TOPp->" << boost::replace_all_copy(it->get_hierarchy(), ".", "->") << ", " << i
              << ", " << it->mElData.nmbBits << ", " << it->mElData.nmbonedimBits << ");" << std::endl;
            break;
        default:
            util::logging::log(util::logging::ERROR,
                               std::string("CType dimensions of injection target not supported: ") +
                                   it->mElData.vrtlCxxType);
            break;
        }
        // x << "	td_.insert( std::make_pair(\"" << it->get_hierarchyDedotted() <<"\", std::move(x" << i << ") ) );"
        // << std::endl;
        x << "	td_.insert( std::make_pair(\"" << it->get_hierarchy() << "\", " << it->get_hierarchyDedotted()
          << "_ ) );" << std::endl;
        // entries << "\t" << "entries_.push_back(" << gen.get_targetdictionaryTargetClassDeclName(*it) << "); \n";
        ++i;
    }

    x << "}" << std::endl;
    x << gen.mTopTypeName << "VRTLmodAPI::~" << gen.mTopTypeName << "VRTLmodAPI(void) { \n\
} \n" << std::endl;

    body_ = x.str();
}

} // namespace vapi
