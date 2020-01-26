////////////////////////////////////////////////////////////////////////////////
/// @file test.cpp
/// @date Created on Mon Jan 25 19:29:05 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtl/VRTLmodAPI/vrtlmod_api.hpp"
#include "vrtl/Vfiapp.h"
#include "verilated.h"

#include "iostream"


VRTLmodAPI& gFrame = VRTLmodAPI::i();
Vfiapp& gVtop = VRTLmodAPI::i().mVRTL;


void clockspin(void){
	gVtop.eval();
	gVtop.clk = 1;
	gVtop.eval();
	gVtop.clk = 0;
}

bool testinject(TDentry& target){
	int cntr = 0;
	std::cout << "\033[1;37mTesting Injection in:\033[0m " << target.name << std::endl;
	target.enable = true;
	target.set_maskBit(1);
	cntr = target.cntr;
	clockspin();
	int cntrdiff = cntr - target.cntr;
	if(cntrdiff >= 0){
		std::cout << "|-> \033[0;31mFailed\033[0m - No injection" << std::endl;
		return false;
	}else if(cntrdiff < -1){
		std::cout << "|-> \033[0;31mFailed\033[0m - More than one injection: " << cntrdiff << std::endl;
		return false;
	}else{
		std::cout << "|-> \033[0;32mPassed\033[0m" << std::endl;
	}
	return true;
}

int main(void){

	// VRTL warum-up
	gVtop.eval();
	VRTLmodAPI::i().mVRTL.reset = 0;

	//test injections
	bool testreturn = false;
	for(auto &it: gFrame.mEntryList){
		testreturn = testinject(*it);
	}

}
