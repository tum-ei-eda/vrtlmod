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
"" << gen.mTopTypeName << "VRTLmodAPI::" << gen.mTopTypeName << "VRTLmodAPI(void) \n\
	: TD_API() { \n\
		vrtl_ = std::make_unique<" << gen.mTopTypeName << ">(";
	if(gen.is_systemc()) {
		x << "\"" << gen.mTopTypeName << "\"";
	}
	x << "); \n"
//} \n"
		<< std::endl
		<<
//"void TD_API::init(std::unique_ptr<" << gen.mTopTypeName << ">&& vrtl) { \n\
	vrtl_ = std::move(vrtl);\n\
	mTD = new sTD(" << std::endl;
"	auto td = std::make_unique<"<< gen.mTopTypeName<< "_TD>(\n";
	bool first = true;
	for (auto const &it : gen.mTargets) {
		if (first) {
			first = false;
		} else {
			x << "," << std::endl;
		}
		x << "\t\t std::make_shared<" << gen.get_targetdictionaryTargetClassDefName(*it) << ">(\"" << it->mElData.hierarchy << "\", ";
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
	x << "\n\t); \n";
/*	x << "\n\
	int i = 0; \n" << std::endl;
	for (auto const &it : gen.mTargets) {
	x << "\
	i = push((td->" << gen.get_targetdictionaryTargetClassDeclName(*it) << ")); \n\
	if( i > 0 ) { \n\
		std::cout << \"ERROR: " <<  gen.mTopTypeName << "VRTLmodAPI target registered multiple times [" << gen.get_targetdictionaryTargetClassDeclName(*it) << "]\" << std::endl; \n\
	}" << std::endl;
} */
	x <<
"	td_ = std::move(td); \n\
}\n" << std::endl;

	x << gen.mTopTypeName << "VRTLmodAPI::~" << gen.mTopTypeName << "VRTLmodAPI(void) { \n\
	vrtl_.reset(nullptr); \n\
} \n" << std::endl;

	entries << gen.mTopTypeName << "_TD::" << gen.mTopTypeName << "_TD(" << std::endl;
	first = true;
	for (auto const &it : gen.mTargets) {
		if (first) {
			first = false;
		} else {
			entries << ", " << std::endl;
		}
		entries << "\tstd::shared_ptr<" << gen.get_targetdictionaryTargetClassDefName(*it) << "> a" << it->index;
	}
	entries << ") : " << std::endl;
	first = true;
	for (auto const &it :	gen.mTargets) {
		if (first) {
			first = false;
		} else {
			entries << "," << std::endl;
		}
		entries << "\t\t" << gen.get_targetdictionaryTargetClassDeclName(*it) << "(a" << it->index << ")";
	}
	entries << " { \n";
	for (auto const &it :	gen.mTargets) {
		entries << "\t" << "entries_.push_back(" << gen.get_targetdictionaryTargetClassDeclName(*it) << "); \n";
	}
	entries << "}" << std::endl;

	x << entries.str();

	body_ = x.str();

}

} // namespace vapi
