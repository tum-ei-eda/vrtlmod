#include "verilated.h"
#include "TD/targetdictionary.hpp"
#include "vrtlmod_api.hpp"

TDentry::TDentry(const char* name, const unsigned index, const unsigned bits) :
	name(name), index(index), cntr(), enable(false), inj_type(INJ_TYPE::BITFLIP), bits(bits) {
}

int TD_API::get_EntryArrayIndex(const char* targetname) const{
	unsigned int i= 0;
	for(auto & it: mEntryList){
		if(std::strcmp(targetname, it->name) == 0){
			return i;
		}
		i++;
	}
	return (-1);
}

int TD_API::get_EntryArrayIndex(const unsigned targetindex) const{
	unsigned int i= 0;
	for(auto & it: mEntryList){
		if(it->index == targetindex){
			return i;
		}
		i++;
	}
	return (-1);
}

int TD_API::prep_inject(const char* targetname, unsigned bit, INJ_TYPE_t type){
	int ret = 0;
	if (type != INJ_TYPE::BITFLIP){
		ret |= BIT_CODES::ERROR_INJTYPE_UNSUPPORTED;
	}
	const int tIndex = get_EntryArrayIndex(targetname);
	if(tIndex < 0){
		ret |= BIT_CODES::ERROR_TARGET_NAME_UNKNOWN;
	}
	if(ret != 0){
		return (ret);
	}else{
		unsigned index = static_cast<unsigned>(tIndex);
		return(prep_inject(index, bit, type));
	}
}

int TD_API::prep_inject(const unsigned targetindex, const unsigned bit, INJ_TYPE_t type){
	int ret = 0;
	if (type != INJ_TYPE::BITFLIP){
		ret |= BIT_CODES::ERROR_INJTYPE_UNSUPPORTED;
	}
	const int tIndex = get_EntryArrayIndex(targetindex);
	if(tIndex < 0){
		ret |= BIT_CODES::ERROR_TARGET_IDX_UNKNOWN;
	}
	if(ret != 0){
		return (ret);
	}else{
		unsigned index = static_cast<unsigned>(tIndex);
		if(bit > mEntryList[index]->bits -1){
			return BIT_CODES::ERROR_BIT_OUTOFRANGE;
		}
		mEntryList[index]->inj_type = type;
		mEntryList[index]->set_maskBit(bit);
		mEntryList[index]->cntr = 0;
	}
	return (BIT_CODES::SUCC_TARGET_ARMED);
}

int TD_API::reset_inject(const char* targetname){
	const int tIndex = get_EntryArrayIndex(targetname);
	if(tIndex < 0){
		return(BIT_CODES::ERROR_TARGET_NAME_UNKNOWN);
	}else{
		unsigned index = static_cast<unsigned>(tIndex);
		return(reset_inject(index));
	}
}

int TD_API::reset_inject(const unsigned targetindex){
	const int tIndex = get_EntryArrayIndex(targetindex);
	if(tIndex < 0){
		return(BIT_CODES::ERROR_TARGET_IDX_UNKNOWN);
	}else{
		unsigned index = static_cast<unsigned>(tIndex);
		mEntryList[index]->enable =false;
		mEntryList[index]->cntr = 0;
		mEntryList[index]->reset_mask();
		return (BIT_CODES::SUCC_TARGET_DISARMED);
	}
}
