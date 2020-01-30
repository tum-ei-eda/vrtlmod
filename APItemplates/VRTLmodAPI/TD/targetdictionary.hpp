//<INSERT_HEADER_COMMMENT>

#ifndef TARGETDICTIONARY_H
#define TARGETDICTIONARY_H

#include <vector>
#include "verilated.h"
//<INSERT_TOP_INCLUDE>

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

	TDentry(const char* name, const unsigned index, const unsigned bits);
	virtual ~TDentry(void){}
};

//<INSERT_TD_CLASSES>

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
	void init(<INSERT_VTOPTYPE>& pVRTL);


	TD_API(void): mTD(), mEntryList(){}
private:
	int get_EntryArrayIndex(const char* targetname) const;
	int get_EntryArrayIndex(const unsigned targetindex) const;

};

#endif //TARGETDICTIONARY_H
