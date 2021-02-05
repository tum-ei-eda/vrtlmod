////////////////////////////////////////////////////////////////////////////////
/// @file vrtlmodapi_source.cpp
/// @date Created on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vapi/generator.hpp"

namespace vapi {

void VapiGenerator::VapiSource::generate_body(void){
	std::stringstream x, entries;
	VapiGenerator& gen = VapiGenerator::_i();

	x <<
"// Vrtl-specific includes: \n\
#include \"" << gen.get_vrtltopheader_filename() << "\" \n\
#include \"" << gen.get_vrtltopsymsheader_filename() << "\" \n\
// General API includes: \n\
#include <memory> \n\
#include <iostream> \n\
#include \"verilated.h\" \n\
#include \"" << gen.get_targetdictionary_relpath() << "\" \n\
#include \"" << gen.get_apiheader_filename() << "\" \n"
		<< std::endl
		<<
"TDentry::TDentry(const char* name, const unsigned index, const unsigned bits) \n\
	:	name(name) \n\
	, index(index) \n\
	, cntr() \n\
	, enable(false) \n\
	, inj_type(INJ_TYPE::BITFLIP) \n\
	, bits(bits) { }"
		<< std::endl
		<<
"int TD_API::get_EntryArrayIndex(const char* targetname) const { \n\
	unsigned int i= 0; \n\
	for(auto & it: mEntryList) { \n\
		if(std::strcmp(targetname, it->name) == 0) { \n\
			return i; \n\
		} \n\
		++i; \n\
	} \n\
	return (-1); \n\
}"
		<< std::endl
		<<
"int TD_API::get_EntryArrayIndex(const unsigned targetindex) const { \n\
	unsigned int i= 0; \n\
	for(auto & it: mEntryList) { \n\
		if(it->index == targetindex) { \n\
			return i; \n\
		} \n\
		++i; \n\
	} \n\
	return (-1); \n\
}"
		<< std::endl
		<<
"int TD_API::prep_inject(const char* targetname, unsigned bit, INJ_TYPE_t type) { \n\
	int ret = 0; \n\
	if (type != INJ_TYPE::BITFLIP) { \n\
		ret |= BIT_CODES::ERROR_INJTYPE_UNSUPPORTED; \n\
	} \n\
	const int tIndex = get_EntryArrayIndex(targetname); \n\
	if(tIndex < 0) { \n\
		ret |= BIT_CODES::ERROR_TARGET_NAME_UNKNOWN; \n\
	} \n\
	if(ret != 0) { \n\
		return (ret); \n\
	} else { \n\
		unsigned index = static_cast<unsigned>(tIndex); \n\
		return(prep_inject(index, bit, type)); \n\
	} \n\
}"
		<< std::endl
		<<
"int TD_API::prep_inject(const unsigned targetindex, const unsigned bit, INJ_TYPE_t type) { \n\
	int ret = 0; \n\
	if (type != INJ_TYPE::BITFLIP) { \n\
		ret |= BIT_CODES::ERROR_INJTYPE_UNSUPPORTED; \n\
	} \n\
	const int tIndex = get_EntryArrayIndex(targetindex); \n\
	if(tIndex < 0) { \n\
		ret |= BIT_CODES::ERROR_TARGET_IDX_UNKNOWN; \n\
	} \n\
	if(ret != 0) { \n\
		return (ret); \n\
	} else { \n\
		unsigned index = static_cast<unsigned>(tIndex); \n\
		if(bit > mEntryList[index]->bits -1) { \n\
			return BIT_CODES::ERROR_BIT_OUTOFRANGE; \n\
		} \n\
		mEntryList[index]->inj_type = type; \n\
		mEntryList[index]->set_maskBit(bit); \n\
		mEntryList[index]->cntr = 0; \n\
	} \n\
	return (BIT_CODES::SUCC_TARGET_ARMED); \n\
}"
		<< std::endl
		<<
"int TD_API::reset_inject(const char* targetname) { \n\
	const int tIndex = get_EntryArrayIndex(targetname); \n\
	if(tIndex < 0) { \n\
		return(BIT_CODES::ERROR_TARGET_NAME_UNKNOWN); \n\
	} else { \n\
		unsigned index = static_cast<unsigned>(tIndex); \n\
		return(reset_inject(index)); \n\
	} \n\
}"
		<< std::endl
		<<
"int TD_API::reset_inject(const unsigned targetindex) { \n\
	const int tIndex = get_EntryArrayIndex(targetindex); \n\
	if(tIndex < 0) { \n\
		return(BIT_CODES::ERROR_TARGET_IDX_UNKNOWN); \n\
	} else { \n\
		unsigned index = static_cast<unsigned>(tIndex); \n\
		mEntryList[index]->enable =false; \n\
		mEntryList[index]->cntr = 0; \n\
		mEntryList[index]->reset_mask(); \n\
		return (BIT_CODES::SUCC_TARGET_DISARMED); \n\
	} \n\
}"
		<< std::endl
		<< std::endl
		<<
"" << gen.mTopTypeName << "VRTLmodAPI::" << gen.mTopTypeName << "VRTLmodAPI(void) \n\
	: TD_API() { \n\
		init(std::move(std::make_unique<" << gen.mTopTypeName << ">(";
	if(gen.is_systemc()) {
		x << "\"" << gen.mTopTypeName << "\"";
	}
	x << "))); \n\
} \n"
		<< std::endl
		<<
"void TD_API::init(std::unique_ptr<" << gen.mTopTypeName << ">&& vrtl) { \n\
	vrtl_ = std::move(vrtl);\n\
	mTD = new sTD(" << std::endl;

	bool first = true;
	for (auto const &it : gen.mTargets) {
		if (first) {
			first = false;
		} else {
			x << "," << std::endl;
		}
		x << "\t\t* new " << gen.get_targetdictionaryTargetClassDefName(*it) << "(\"" << it->mElData.hierarchy << "\", ";
		std::string hier = it->get_hierarchy();
		auto fdot = hier.find(".");
		if (fdot != std::string::npos) {
			if (it->mElData.words > 1) {
				x << "vrtl_" << "->__VlSymsp->TOPp->" << hier.substr(0, fdot) << "->" << hier.substr(fdot + 1);
			} else {
				x << "&(" << "vrtl_" << "->__VlSymsp->TOPp->" << hier.substr(0, hier.find(".")) << "->" << hier.substr(hier.find(".") + 1) << ")";
			}
		} else {
			if (it->mElData.words > 1) {
				x << "vrtl_" << "->__VlSymsp->TOPp->" << hier;
			} else {
				x << "&(" << "vrtl_" << "->__VlSymsp->TOPp->" << hier << ")";
			}
		}
		x << ")";
	}
	x << std::endl << "\t); \n\
	int i = 0; \n" << std::endl;
	for (auto const &it : gen.mTargets) {
	x << "\
	i = push(&(mTD->" << gen.get_targetdictionaryTargetClassDeclName(*it) << ")); \n\
	if( i > 0 ) { \n\
		std::cout << \"ERROR: " <<  gen.mTopTypeName << "VRTLmodAPI target registered multiple times [" << gen.get_targetdictionaryTargetClassDeclName(*it) << "]\" << std::endl; \n\
	}" << std::endl;
	}
	x << "}";

	body_ = x.str();
}

} // namespace vapi
