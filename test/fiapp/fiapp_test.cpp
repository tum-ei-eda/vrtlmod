////////////////////////////////////////////////////////////////////////////////
/// @file test.cpp
/// @date Created on Mon Jan 25 19:29:05 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmodapi/Vfiapp_vrtlmodapi.hpp"
#include "Vfiapp.h"
#include "verilated.h"

#include <iostream>
#include <sstream>

VfiappVRTLmodAPI& gFrame = VfiappVRTLmodAPI::i();
#define gVtop (*(gFrame.vrtl_))

void clockspin(void){
	gVtop.eval();
	gVtop.clk = 1;
	gVtop.eval();
	gVtop.clk = 0;
}

bool testinject(vrtlfi::td::TDentry& target, vrtlfi::td::TD_API& api){
	std::cout << "\033[1;37mTesting Injection in:\033[0m " << target.get_name() << std::endl;
	int cntrsum = 0, cntrsum_new = 0;
	std::vector<int> cntr = target.get_cntr();
	api.prep_inject(target.get_name(), 0);
	target.arm();
	clockspin();
	std::vector<int> cntr_new = target.get_cntr();

	std::stringstream x, y;
	std::cout << "CO:\t";
	for(const auto& it: cntr) {
		x << "|" << it;
		cntrsum += it;
	}
	std::cout << x.str() << "|" << std::endl << "CN:\t";
	for(const auto& it: cntr_new) {
		y << "|" << it;
		cntrsum_new += it;
	}
	std::cout << y.str() << "|" << std::endl;

	if(cntrsum != 0) {
		std::cout << "|-> \033[0;31mFailed\033[0m - Target dictionary in corrupt state" << std::endl;
		return false;
	}
	if(cntrsum_new > 1) {
		std::cout << "|-> \033[0;31mFailed\033[0m - More than one injection: " << cntrsum_new << std::endl;
		return false;
	}
	if(cntrsum_new == 0) {
		std::cout << "|-> \033[0;31mFailed\033[0m - No injection" << std::endl;
		return false;
	}

	std::cout << "|-> \033[0;32mPassed\033[0m" << std::endl;
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
		testreturn &= testinject( *(it.second), gFrame );
	}
	if(testreturn && gFrame.td_.size() > 0){
		std::cout << std::endl << "..." << "All passed. \033[0;32mTest successful.\033[0m" << std::endl;
	}else {
		std::cout << std::endl << "..." << "\033[0;31mTest failed.\033[0m" << std::endl;
		return(1);
	}
	return(0);
}
