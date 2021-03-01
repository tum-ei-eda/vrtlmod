////////////////////////////////////////////////////////////////////////////////
/// @file test.cpp
/// @date Created on Mon Jan 25 19:29:05 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/Vfiapp_vrtlmodapi.hpp"
#include "Vfiapp.h"
#include "verilated.h"

#include "iostream"

VfiappVRTLmodAPI& gFrame = VfiappVRTLmodAPI::i();
#define gVtop (*(gFrame.vrtl_))

void clockspin(void){
	gVtop.eval();
	gVtop.clk = 1;
	gVtop.eval();
	gVtop.clk = 0;
}

bool testinject(TDentry& target){
	int cntr = 0;
	std::cout << "\033[1;37mTesting Injection in:\033[0m " << target.get_name() << std::endl;
	target.enable_ = true;
	target.set_maskBit(1);
	cntr = target.cntr_;
	clockspin();
	int cntrdiff = cntr - target.cntr_;
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
	gVtop.reset = 0;
	std::cout << std::endl << "Running test for simple fault injection application (fiapp)" << "..." << std::endl;
	//test injections
	bool testreturn = true;
	for(auto &it: gFrame.td_){
		testreturn &= testinject( *(it.second) );
	}
	if(testreturn && gFrame.td_.size() > 0){
		std::cout << std::endl << "..." << "All passed. \033[0;32mTest successful.\033[0m" << std::endl;
	}else {
		std::cout << std::endl << "..." << "\033[0;31mTest failed.\033[0m" << std::endl;
		return(1);
	}
	return(0);
}
