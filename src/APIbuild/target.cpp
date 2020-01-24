////////////////////////////////////////////////////////////////////////////////
/// @file target.cpp
/// @date Created on Mon Jan 10 18:21:20 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include <APIbuild/target.hpp>
#include <sstream>

bool strhelp::replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void strhelp::replaceAll(std::string& str, const std::string& from, const std::string& to) {
	while(replace(str, from, to)){}
}


std::string Target::get_hierarchy(void) {
	std::string ret = mElData.hierarchy.substr(mElData.hierarchy.find(".") + 1);
	return (ret);
}

std::string Target::get_hierarchyDedotted(void) {
	std::string ret = get_hierarchy();
	if (ret.find(".") != std::string::npos) {
		strhelp::replaceAll(ret, std::string("."), std::string("__DOT__") );// ret.replace(ret.begin(), ret.end(), ".", );
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
	ret << "\t" << mElData.hierarchy << "[" << mElData.nmbBits << "] " << mElData.type;
	return (ret.str());
}

Target::Target(unsigned int index, sXmlEl_t &data)
		: mSeqInjCnt(), index(index), mElData(data), mTD_typedef(), mTD_decl() {
}

