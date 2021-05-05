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
	std::shared_ptr<"	<<  gen.mTopTypeName  << "> vrtl_{nullptr}; \n\
	virtual ~" << gen.mTopTypeName << "VRTLmodAPI(void);\n\n";
	
	
	for (auto const &it : gen.mTargets) {
		if(it->mSeqInjCnt == 0) 
			continue;
		x << 
"	std::shared_ptr<TDentry> " << it->get_hierarchyDedotted() << "_{};\n";
	}
		
	x << std::endl << "}; \n"
"#endif /* __" << gen.mTopTypeName << "VRTLMODAPI_VRTLMODAPI_HPP__ */";

	body_ = x.str();
}

} // namespace vapi
