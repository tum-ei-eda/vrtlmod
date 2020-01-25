//<INSERT_HEADER_COMMMENT>

#ifndef APITEMPLATES_VRTLMODAPI_VRTLMOD_API_HPP_
#define APITEMPLATES_VRTLMODAPI_VRTLMOD_API_HPP_

#include <vector>
#include "TD/targetdictionary.hpp"

class <INSERT_VTOPTYPE>;

class VRTLmodAPI : public TD_API {
public:
	<INSERT_VTOPTYPE>& mVRTL;
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
