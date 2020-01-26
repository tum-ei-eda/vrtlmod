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

//<INSERT_TD_CLASSES>

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
	void init(<INSERT_VTOPTYPE>& pVRTL);
	TD_API(void): mTD(), mEntryList(){}
};

#endif //TARGETDICTIONARY_H
