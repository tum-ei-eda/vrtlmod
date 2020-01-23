////////////////////////////////////////////////////////////////////////////////
/// @file target.cpp
/// @date Created on Mon Jan 10 18:21:20 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include <APIbuild/target.hpp>
#include <sstream>

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

Target::Target(unsigned int index, sXmlEl_t& data) : index(index), mElData(data), mTD_typedef(), mTD_decl()
{
}

