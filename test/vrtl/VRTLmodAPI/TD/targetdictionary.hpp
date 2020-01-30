////////////////////////////////////////////////////////////////////////////////
/// @file vrtlmod_api.hpp
/// @brief Modified VRTL-API main header
/// @details Automatically generated from: test/regpicker.xml
/// @date Created on Wed Jan 29 14:04:14 2020
/// @author APIbuilder version 0.9
////////////////////////////////////////////////////////////////////////////////

#ifndef TARGETDICTIONARY_H
#define TARGETDICTIONARY_H

#include <vector>
#include "verilated.h"
#include "Vfiapp.h"

typedef enum INJ_TYPE{BIASED_S, BIASED_R, BITFLIP} INJ_TYPE_t;

class TD_API;

class TDentry{
public:
	unsigned index;
	bool enable;
	const char* name;
	unsigned cntr;
	INJ_TYPE_t inj_type;

	virtual void set_maskBit(unsigned bit){};
	virtual void reset_mask(void){};

	TDentry(const char* name, unsigned index);
	virtual ~TDentry(void){}
};


/* (TDentry-Id 0):register	TOP.o1[1] VL_OUT8 */
class TDentry_o1: public TDentry {
	public:
		unsigned bits;
		CData* data;	// CData
		CData mask;
		void reset_mask(void){mask = 0;}
		void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}
		TDentry_o1(const char* name, CData* data) :
			TDentry(name, 0), data(data), mask(), bits(1) {}
};

/* (TDentry-Id 1):register	TOP.o2[1] VL_OUT8 */
class TDentry_o2: public TDentry {
	public:
		unsigned bits;
		CData* data;	// CData
		CData mask;
		void reset_mask(void){mask = 0;}
		void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}
		TDentry_o2(const char* name, CData* data) :
			TDentry(name, 1), data(data), mask(), bits(1) {}
};

/* (TDentry-Id 2):register	TOP.o3[1] VL_OUT8 */
class TDentry_o3: public TDentry {
	public:
		unsigned bits;
		CData* data;	// CData
		CData mask;
		void reset_mask(void){mask = 0;}
		void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}
		TDentry_o3(const char* name, CData* data) :
			TDentry(name, 2), data(data), mask(), bits(1) {}
};

/* (TDentry-Id 3):register	TOP.fiapp__DOT__q1[1] VL_SIG8 */
class TDentry_fiapp__DOT__q1: public TDentry {
	public:
		unsigned bits;
		CData* data;	// CData
		CData mask;
		void reset_mask(void){mask = 0;}
		void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}
		TDentry_fiapp__DOT__q1(const char* name, CData* data) :
			TDentry(name, 3), data(data), mask(), bits(1) {}
};

/* (TDentry-Id 4):register	TOP.fiapp__DOT__q2[1] VL_SIG8 */
class TDentry_fiapp__DOT__q2: public TDentry {
	public:
		unsigned bits;
		CData* data;	// CData
		CData mask;
		void reset_mask(void){mask = 0;}
		void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}
		TDentry_fiapp__DOT__q2(const char* name, CData* data) :
			TDentry(name, 4), data(data), mask(), bits(1) {}
};

/* (TDentry-Id 5):register	TOP.fiapp__DOT__q3[1] VL_SIG8 */
class TDentry_fiapp__DOT__q3: public TDentry {
	public:
		unsigned bits;
		CData* data;	// CData
		CData mask;
		void reset_mask(void){mask = 0;}
		void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}
		TDentry_fiapp__DOT__q3(const char* name, CData* data) :
			TDentry(name, 5), data(data), mask(), bits(1) {}
};

typedef struct sTD {
	TDentry_o1& e_o1;
	TDentry_o2& e_o2;
	TDentry_o3& e_o3;
	TDentry_fiapp__DOT__q1& e_fiapp__DOT__q1;
	TDentry_fiapp__DOT__q2& e_fiapp__DOT__q2;
	TDentry_fiapp__DOT__q3& e_fiapp__DOT__q3;
	sTD(
		TDentry_o1& a0, 
		TDentry_o2& a1, 
		TDentry_o3& a2, 
		TDentry_fiapp__DOT__q1& a3, 
		TDentry_fiapp__DOT__q2& a4, 
		TDentry_fiapp__DOT__q3& a5) : 
			 e_o1(a0),
			 e_o2(a1),
			 e_o3(a2),
			 e_fiapp__DOT__q1(a3),
			 e_fiapp__DOT__q2(a4),
			 e_fiapp__DOT__q3(a5){}
} sTD_t;

class TD_API{
public:
	sTD_t* mTD;
	std::vector<TDentry*> mEntryList;
	sTD_t& get_struct(void){return(*mTD);}
	int push(TDentry *newEntry) {
		for (auto const & it: mEntryList){
			if(it == newEntry) return (0);
		}
		mEntryList.push_back(newEntry);
		return (1);
	}
	void init(Vfiapp& pVRTL);
	TD_API(void): mTD(), mEntryList(){}
};

#endif //TARGETDICTIONARY_H
