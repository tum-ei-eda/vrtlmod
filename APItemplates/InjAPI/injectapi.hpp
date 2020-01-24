////////////////////////////////////////////////////////////////////////////////
/// @file injectapi.hpp
/// @brief Target Inject API main header.
/// @date Created on Mon Jan 17 08:33:21 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_APIBUILD_INJAPI_INJECTAPI_HPP_
#define INCLUDE_APIBUILD_INJAPI_INJECTAPI_HPP_

#include <vector>

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

class TDentry;

class TD{
public:

	std::vector<TDentry*> mEntryList;

	int push(TDentry* newEntry){
		i().mEntryList.push_back(newEntry);
		return(1);
	}

	static TD& i(void){
		static TD _instance;
		return (_instance);
	}

	TD(void){}
	virtual ~TD(void){}
};

class TDentry{
public:
	unsigned index;
	bool enable;
	const char* name;
	unsigned cntr;
	INJ_TYPE_t inj_type;

	virtual void set_maskBit(unsigned bit){};
	virtual void reset_mask(void){};

	TDentry(const char* name, unsigned index) : name(name), index(index), cntr(), enable(false), inj_type(INJ_TYPE::BITFLIP) {
		TD::i().push(this);
	}
	virtual ~TDentry(void){}
};

#endif /* INCLUDE_APIBUILD_INJAPI_INJECTAPI_HPP_ */
