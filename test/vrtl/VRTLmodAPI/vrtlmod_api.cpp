////////////////////////////////////////////////////////////////////////////////
/// @file vrtlmod_api.hpp
/// @brief Modified VRTL-API main header
/// @details Automatically generated from: test/regpicker.xml
/// @date Created on Wed Jan 29 14:04:14 2020
/// @author APIbuilder version 0.9
////////////////////////////////////////////////////////////////////////////////

// Vrtl-specific includes:
#include "../Vfiapp.h"
#include "../Vfiapp__Syms.h"
// General API includes:
#include "verilated.h"
#include "TD/targetdictionary.hpp"
#include "vrtlmod_api.hpp"

TDentry::TDentry(const char* name, unsigned index) :
	name(name), index(index), cntr(), enable(false), inj_type(INJ_TYPE::BITFLIP) {
//	api.push(this);
}


VRTLmodAPI::VRTLmodAPI(void) :
	mVRTL(* new Vfiapp),
TD_API()
{TD_API::init(mVRTL);}
void TD_API::init(Vfiapp& pVRTL){
mTD = new sTD(
		* new TDentry_o1("TOP.o1", &(pVRTL.__VlSymsp->TOPp->o1)),
		* new TDentry_o2("TOP.o2", &(pVRTL.__VlSymsp->TOPp->o2)),
		* new TDentry_o3("TOP.o3", &(pVRTL.__VlSymsp->TOPp->o3)),
		* new TDentry_fiapp__DOT__q1("TOP.fiapp__DOT__q1", &(pVRTL.__VlSymsp->TOPp->fiapp__DOT__q1)),
		* new TDentry_fiapp__DOT__q2("TOP.fiapp__DOT__q2", &(pVRTL.__VlSymsp->TOPp->fiapp__DOT__q2)),
		* new TDentry_fiapp__DOT__q3("TOP.fiapp__DOT__q3", &(pVRTL.__VlSymsp->TOPp->fiapp__DOT__q3))
	);

	mEntryList.push_back(&(mTD->e_o1));
	mEntryList.push_back(&(mTD->e_o2));
	mEntryList.push_back(&(mTD->e_o3));
	mEntryList.push_back(&(mTD->e_fiapp__DOT__q1));
	mEntryList.push_back(&(mTD->e_fiapp__DOT__q2));
	mEntryList.push_back(&(mTD->e_fiapp__DOT__q3));
}
