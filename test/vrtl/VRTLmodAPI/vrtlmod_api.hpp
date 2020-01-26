////////////////////////////////////////////////////////////////////////////////
/// @file vrtlmod_api.hpp
/// @brief Modified VRTL-API main header
/// @details Automatically generated from: test/regpicker.xml
/// @date Created on Sun Jan 26 20:59:40 2020
/// @author APIbuilder version 0.9
////////////////////////////////////////////////////////////////////////////////

#ifndef APITEMPLATES_VRTLMODAPI_VRTLMOD_API_HPP_
#define APITEMPLATES_VRTLMODAPI_VRTLMOD_API_HPP_

#include <vector>
#include "TD/targetdictionary.hpp"

class Vfiapp;

#define FI_LIKELY(x)   __builtin_expect(!!(x), 1)
#define FI_UNLIKELY(x) __builtin_expect(!!(x), 0)

#define SEQ_TARGET_INJECT(TDentry) { \
	if(FI_UNLIKELY((TDentry).enable)) { \
		if(((TDentry).cntr <= 0) and (TDentry).mask) { \
			if(FI_LIKELY(((TDentry).inj_type == INJ_TYPE::BITFLIP))){ \
				*((TDentry).data) = *((TDentry).data) ^ (TDentry).mask; \
			} \
			(TDentry).cntr++; \
		} \
	} \
}

#define SEQ_TARGET_INJECT_W(TDentry, word) { \
	if(FI_UNLIKELY((TDentry).enable) { \
		if(((TDentry).cntr <= 0) and (TDentry).mask[(word)]) { \
			if(FI_LIKELY(((TDentry).inj_type == INJ_TYPE::BITFLIP))){ \
				((TDentry).data[(word)]) = ((TDentry).data[(word)]) ^ (TDentry).mask[(word)]; \
			} \
			(TDentry).cntr++; \
		} \
	} \
}

#define INT_TARGET_INJECT(TDentry) {\
	SEQ_TARGET_INJECT(TDentry) \
}
#define INT_TARGET_INJECT_W(TDentry, words) {\
	for(unsigned i = 0; i < words; ++i) { \
		SEQ_TARGET_INJECT_W(TDentry, i) }\
}

class VRTLmodAPI : public TD_API {
public:
	Vfiapp& mVRTL;
//	TD_API& mTargetDictionary;

	static VRTLmodAPI& i(void) {
		static VRTLmodAPI _instance;
		return (_instance);
	}

private:
	VRTLmodAPI(void);
	VRTLmodAPI(VRTLmodAPI const&);
	void operator=(VRTLmodAPI const&);
public:
	virtual ~VRTLmodAPI(void) {
	}
};

#endif /* APITEMPLATES_VRTLMODAPI_VRTLMOD_API_HPP_ */
