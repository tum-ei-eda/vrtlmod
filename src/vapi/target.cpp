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
/// @file target.cpp
/// @date Created on Mon Jan 10 18:21:20 2020
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vapi/target.hpp"
#include "vrtlmod/util/system.hpp"

#include <sstream>

namespace vapi {

std::string Target::get_hierarchy(void) {
	std::string ret = mElData.hierarchy.substr(mElData.hierarchy.find(".") + 1);
	return (ret);
}

std::string Target::get_hierarchyDedotted(void) {
	std::string ret = get_hierarchy();
	if (ret.find(".") != std::string::npos) {
		util::strhelp::replaceAll(ret, std::string("."), std::string("__DOT__"));
	}
	return (ret);
}

std::string Target::_self(void) const {
	std::stringstream ret;
	switch (mElData.signalClass) {
	case sXmlEl::REG:
		ret << "register";
		break;
	case sXmlEl::WIRE:
		ret << "wire";
		break;
	case sXmlEl::CONST:
		ret << "constant";
		break;
	default:
		break;
	}
	ret << "\t" << mElData.hierarchy << "[" << mElData.nmbBits << "] " << mElData.cxxbasetype_;
	return (ret.str());
}

Target::Target(unsigned int index, sXmlEl_t &data) :
		mSeqInjCnt(0), index(index), mElData(data), mTD_typedef(), mTD_decl() {
}

ExprT::ExprT(const char *Expr) :
		expr(Expr), prefix(), object(), name() {
	auto pos = expr.find("->");
	if (pos != std::string::npos)
		prefix = expr.substr(0, pos);

	auto objdot = expr.rfind(".");
	if (objdot != std::string::npos) {
		object = expr.substr(pos + 2, objdot - pos - 2);
		util::strhelp::replaceAll(object, std::string("__"), std::string("."));
		name = expr.substr(objdot + 1);
	} else
		name = expr.substr(pos + 2);
}

} // namespace vapi
