//<INSERT_HEADER_COMMMENT>

#ifndef TARGETDICTIONARY_H
#define TARGETDICTIONARY_H

#include <vector>
#include "verilated.h"
//<INSERT_TOP_INCLUDE>

typedef enum INJ_TYPE{BIASED_S, BIASED_R, BITFLIP} INJ_TYPE_t;

#define FI_LIKELY(x)   __builtin_expect(!!(x), 1)
#define FI_UNLIKELY(x) __builtin_expect(!!(x), 0)

#define SEQ_TARGET_INJECT(TDentry) { \
	if(FI_UNLIKELY((TDentry).enable)) { \
		if(FI_LIKELY(((TDentry).inj_type == INJ_TYPE::BITFLIP))){ \
			*((TDentry).data) = *((TDentry).data) ^ (TDentry).mask; \
		} \
		(TDentry).cntr++; \
	} \
}

#define SEQ_TARGET_INJECT_W(TDentry, word) { \
	if(FI_UNLIKELY((TDentry).enable and (TDentry).mask[(word)])) { \
		if(FI_LIKELY(((TDentry).inj_type == INJ_TYPE::BITFLIP))){ \
			((TDentry).data[(word)]) = ((TDentry).data[(word)]) ^ (TDentry).mask[(word)]; \
		} \
		(TDentry).cntr++; \
	} \
}

#define INT_TARGET_INJECT(TDentry) {\
	SEQ_TARGET_INJECT(TDentry) \
}
#define INT_TARGET_INJECT_W(TDentry, words) {\
	for(unsigned i = 0; i < words; ++i) { \
		SEQ_TARGET_INJECT_W(TDentry, i) }\
}

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
