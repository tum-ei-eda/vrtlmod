////////////////////////////////////////////////////////////////////////////////
/// @file vrtlmod_api.hpp
/// @brief Modified VRTL-API main header
/// @details Automatically generated from: test/regpicker.xml
/// @date Created on Fri Feb 14 17:21:20 2020
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
	const unsigned index;
	bool enable;
	const char* name;
	unsigned cntr;
	INJ_TYPE_t inj_type;
	const unsigned bits;

	void arm(void){enable=true;}
	void disarm(void){enable=false;}

	virtual void set_maskBit(unsigned bit){};
	virtual void reset_mask(void){};

	virtual void read_data(uint8_t* pData){};

	TDentry(const char* name, const unsigned index, const unsigned bits);
	virtual ~TDentry(void){}
};


/* (TDentry-Id 0):register	TOP.o1[1] VL_OUT8 */
class TDentry_o1: public TDentry {
	public:
		CData* data;	// CData
		CData mask;
		void reset_mask(void){mask = 0;}
		void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}
void read_data(uint8_t* pData) { 
				unsigned byte = 0; 
				uint8_t* xData = reinterpret_cast<uint8_t*>(data); 
				for(unsigned bit = 0; bit < bits; bit++){ 
					if((bit % 8)==0){ 
						pData[byte] = xData[byte]; 
						byte++; 
					} 
				} 
			}
		TDentry_o1(const char* name, CData* data) :
			TDentry(name, 0, 1), data(data), mask() {}
};

/* (TDentry-Id 1):register	TOP.o2[1] VL_OUT8 */
class TDentry_o2: public TDentry {
	public:
		CData* data;	// CData
		CData mask;
		void reset_mask(void){mask = 0;}
		void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}
void read_data(uint8_t* pData) { 
				unsigned byte = 0; 
				uint8_t* xData = reinterpret_cast<uint8_t*>(data); 
				for(unsigned bit = 0; bit < bits; bit++){ 
					if((bit % 8)==0){ 
						pData[byte] = xData[byte]; 
						byte++; 
					} 
				} 
			}
		TDentry_o2(const char* name, CData* data) :
			TDentry(name, 1, 1), data(data), mask() {}
};

/* (TDentry-Id 2):register	TOP.o3[1] VL_OUT8 */
class TDentry_o3: public TDentry {
	public:
		CData* data;	// CData
		CData mask;
		void reset_mask(void){mask = 0;}
		void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}
void read_data(uint8_t* pData) { 
				unsigned byte = 0; 
				uint8_t* xData = reinterpret_cast<uint8_t*>(data); 
				for(unsigned bit = 0; bit < bits; bit++){ 
					if((bit % 8)==0){ 
						pData[byte] = xData[byte]; 
						byte++; 
					} 
				} 
			}
		TDentry_o3(const char* name, CData* data) :
			TDentry(name, 2, 1), data(data), mask() {}
};

/* (TDentry-Id 3):register	TOP.fiapp__DOT__q1[1] VL_SIG8 */
class TDentry_fiapp__DOT__q1: public TDentry {
	public:
		CData* data;	// CData
		CData mask;
		void reset_mask(void){mask = 0;}
		void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}
void read_data(uint8_t* pData) { 
				unsigned byte = 0; 
				uint8_t* xData = reinterpret_cast<uint8_t*>(data); 
				for(unsigned bit = 0; bit < bits; bit++){ 
					if((bit % 8)==0){ 
						pData[byte] = xData[byte]; 
						byte++; 
					} 
				} 
			}
		TDentry_fiapp__DOT__q1(const char* name, CData* data) :
			TDentry(name, 3, 1), data(data), mask() {}
};

/* (TDentry-Id 4):register	TOP.fiapp__DOT__q2[1] VL_SIG8 */
class TDentry_fiapp__DOT__q2: public TDentry {
	public:
		CData* data;	// CData
		CData mask;
		void reset_mask(void){mask = 0;}
		void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}
void read_data(uint8_t* pData) { 
				unsigned byte = 0; 
				uint8_t* xData = reinterpret_cast<uint8_t*>(data); 
				for(unsigned bit = 0; bit < bits; bit++){ 
					if((bit % 8)==0){ 
						pData[byte] = xData[byte]; 
						byte++; 
					} 
				} 
			}
		TDentry_fiapp__DOT__q2(const char* name, CData* data) :
			TDentry(name, 4, 1), data(data), mask() {}
};

/* (TDentry-Id 5):register	TOP.fiapp__DOT__q3[1] VL_SIG8 */
class TDentry_fiapp__DOT__q3: public TDentry {
	public:
		CData* data;	// CData
		CData mask;
		void reset_mask(void){mask = 0;}
		void set_maskBit(unsigned bit){VL_ASSIGNBIT_IO(1, bit, mask, 0);}
void read_data(uint8_t* pData) { 
				unsigned byte = 0; 
				uint8_t* xData = reinterpret_cast<uint8_t*>(data); 
				for(unsigned bit = 0; bit < bits; bit++){ 
					if((bit % 8)==0){ 
						pData[byte] = xData[byte]; 
						byte++; 
					} 
				} 
			}
		TDentry_fiapp__DOT__q3(const char* name, CData* data) :
			TDentry(name, 5, 1), data(data), mask() {}
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
	typedef enum BIT_CODES{
		ERROR_TARGET_NAME_UNKNOWN = 0x0001,
		ERROR_TARGET_IDX_UNKNOWN = 0x0002,
		ERROR_BIT_OUTOFRANGE = 0x0004,
		ERROR_INJTYPE_UNSUPPORTED = 0x0008,
		SUCC_TARGET_ARMED = 0x10,
		SUCC_TARGET_DISARMED = 0x20
	}BIT_CODES_t;

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

	int prep_inject(const char* targetname, const unsigned bit, const INJ_TYPE_t type = BITFLIP);
	int prep_inject(const unsigned targetindex, const unsigned bit, const INJ_TYPE_t type = BITFLIP);
	int reset_inject(const char* targetname);
	int reset_inject(const unsigned targetindex);
	void init(Vfiapp& pVRTL);

	int get_EntryArrayIndex(const char* targetname) const;
	int get_EntryArrayIndex(const unsigned targetindex) const;

	TD_API(void): mTD(), mEntryList(){}
private:


};

#endif //TARGETDICTIONARY_H
