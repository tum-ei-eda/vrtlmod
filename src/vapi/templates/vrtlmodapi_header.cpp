////////////////////////////////////////////////////////////////////////////////
/// @file vrtlmodapi_header.cpp
/// @date Created on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vapi/generator.hpp"

namespace vapi {

void VapiGenerator::VapiHeader::generate_body(void){
	std::stringstream x, entries;
	VapiGenerator& gen = VapiGenerator::_i();

	for (auto const &it : gen.mTargets) {
		entries << std::endl << gen.get_targetdictionaryEntryTypeDefString(*it);
	}
	entries << std::endl << "typedef struct "<< gen.mTopTypeName << "_TD : TD {" << std::endl;
	for (auto const &it : gen.mTargets) {
		entries << "\tstd::shared_ptr<" << gen.get_targetdictionaryTargetClassDefName(*it) << "> " << gen.get_targetdictionaryTargetClassDeclName(*it) << "{ };"
			<< std::endl;
	}
	entries << "\t"<< gen.mTopTypeName << "_TD(" << std::endl;
	bool first = true;
	for (auto const &it : gen.mTargets) {
		if (first) {
			first = false;
		} else {
			entries << ", " << std::endl;
		}
		entries << "\t\tstd::shared_ptr<" << gen.get_targetdictionaryTargetClassDefName(*it) << "> a" << it->index;
	}
	entries << ");\n" << std::endl;
	entries << "} "<< gen.mTopTypeName << "_TD;" << std::endl;

	x <<
"#ifndef __" << gen.mTopTypeName << "VRTLMODAPI_VRTLMODAPI_HPP__ \n\
#define __" << gen.mTopTypeName << "VRTLMODAPI_VRTLMODAPI_HPP__ \n"
		<< std::endl
		<<
"#include <vector> \n\
#include <memory> \n\
#include \"verilated.h\" \n\
#include \"" << gen.get_targetdictionary_relpath() << "\" \n"
		<<
"class " << gen.mTopTypeName << ";" << std::endl
		<< std::endl
		<< std::endl
		<< entries.str() << std::endl
		<<
"class " << gen.mTopTypeName << "VRTLmodAPI : public TD_API { \n\
public: \n\
	static " << gen.mTopTypeName << "VRTLmodAPI& i(void) { \n\
		static " << gen.mTopTypeName << "VRTLmodAPI _instance; \n\
		return (_instance); \n\
	} \n\
	private: \n\
	" << gen.mTopTypeName << "VRTLmodAPI(void); \n\
	" << gen.mTopTypeName << "VRTLmodAPI(" << gen.mTopTypeName << "VRTLmodAPI const&); \n\
	void operator=(" << gen.mTopTypeName << "VRTLmodAPI const&); \n\
public: \n\
	std::unique_ptr<"	<<  gen.mTopTypeName  << "> vrtl_{nullptr}; \n\
"	<< gen.mTopTypeName <<"_TD& get_struct(void){ return(dynamic_cast<"<< gen.mTopTypeName <<"_TD&>(*td_) ); } \n\
	virtual ~" << gen.mTopTypeName << "VRTLmodAPI(void);\n\
}; \n"
		<< std::endl <<
"#endif /* __" << gen.mTopTypeName << "VRTLMODAPI_VRTLMODAPI_HPP__ */";

	body_ = x.str();
}

} // namespace vapi
