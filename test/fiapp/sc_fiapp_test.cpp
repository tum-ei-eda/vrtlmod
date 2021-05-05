////////////////////////////////////////////////////////////////////////////////
/// @file test.cpp
/// @date Created on Mon Jan 25 19:29:05 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/Vfiapp_vrtlmodapi.hpp"
#include "verilated.h"
#include "systemc.h"
#include "Vfiapp.h"

#include <iostream>
#include <sstream>

VfiappVRTLmodAPI& gFrame = VfiappVRTLmodAPI::i();
#define gVtop (*(gFrame.vrtl_))

sc_signal<bool> tb_clk{"clk"};
sc_signal<bool> tb_reset{"reset"};
sc_signal<bool> tb_a{"a"};
sc_signal<bool> tb_enable{"enable"};
sc_signal<bool> tb_o1{"o1"};
sc_signal<bool> tb_o2{"o2"};
sc_signal<bool> tb_o3{"o3"};

void clockspin(void) {
	static const sc_core::sc_time halfspintime{5.0, sc_core::SC_NS};
	tb_clk.write(0);
	sc_start( halfspintime );
	tb_clk.write(1);
	sc_start( halfspintime );
}

bool testinject(TDentry& target, TD_API& api){
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

int sc_main(int argc, char* argv[]){

	gVtop.clk(tb_clk);
	gVtop.reset(tb_reset);
	gVtop.a(tb_a);
	gVtop.enable(tb_enable);
	gVtop.o1(tb_o1);
	gVtop.o2(tb_o2);
	gVtop.o3(tb_o3);

	// VRTL warum-up
	tb_reset.write(0);
	clockspin();
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
