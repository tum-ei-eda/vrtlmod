////////////////////////////////////////////////////////////////////////////////
/// @file targetdictionary_header.cpp
/// @date Created on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vapi/generator.hpp"

namespace vapi {

void VapiGenerator::TargetDictionary::generate_body(void){
	std::stringstream x, entries;
	VapiGenerator& gen = VapiGenerator::_i();

	for (auto const &it : gen.mTargets) {
		entries << std::endl << gen.get_targetdictionaryEntryTypeDefString(*it);
	}
	entries << std::endl << "typedef struct sTD {" << std::endl;
	for (auto const &it : gen.mTargets) {
		entries << "\t" << gen.get_targetdictionaryTargetClassDefName(*it) << "& " << gen.get_targetdictionaryTargetClassDeclName(*it) << ";"
			<< std::endl;
	}
	entries << "\tsTD(" << std::endl;
	bool first = true;
	for (auto const &it : gen.mTargets) {
		if (first) {
			first = false;
		} else {
			entries << ", " << std::endl;
		}
		entries << "\t\t" << gen.get_targetdictionaryTargetClassDefName(*it) << "& a" << it->index;
	}
	entries << ") : " << std::endl;
	first = true;
	for (auto const &it :	gen.mTargets) {
		if (first) {
			first = false;
		} else {
			entries << "," << std::endl;
		}
		entries << "\t\t\t " << gen.get_targetdictionaryTargetClassDeclName(*it) << "(a" << it->index << ")";
	}
	entries << "{}" << std::endl;
	entries << "} sTD_t;" << std::endl;

	x <<
"#ifndef __VRTLMODAPI_TARGETDICTIONARY_HPP__ \n\
#define __VRTLMODAPI_TARGETDICTIONARY_HPP__ \n\n\
#include <vector> \n\
#include \"verilated.h\" \n\
#include \"" << gen.get_vrtltopheader_filename() << "\" \n\n\
typedef enum INJ_TYPE{BIASED_S, BIASED_R, BITFLIP} INJ_TYPE_t; \n\
class TD_API; \n"
		<< std::endl
		<<
"class TDentry{  \n\
public: \n\
	const unsigned index; \n\
	bool enable; \n\
	const char* name; \n\
	unsigned cntr; \n\
	INJ_TYPE_t inj_type; \n\
	const unsigned bits; \n\n\
	void arm(void){enable=true;} \n\
	void disarm(void){enable=false;} \n\
	virtual void set_maskBit(unsigned bit){}; \n\
	virtual void reset_mask(void){}; \n\
	virtual void read_data(uint8_t* pData){}; \n\
	TDentry(const char* name, const unsigned index, const unsigned bits); \n\
	virtual ~TDentry(void){} \n\
};"
		<< std::endl
		<< entries.str() << std::endl
		<< std::endl
		<<
"class TD_API{ \n\
public: \n\
	typedef enum BIT_CODES{ \n\
		ERROR_TARGET_NAME_UNKNOWN = 0x0001, \n\
		ERROR_TARGET_IDX_UNKNOWN = 0x0002, \n\
		ERROR_BIT_OUTOFRANGE = 0x0004, \n\
		ERROR_INJTYPE_UNSUPPORTED = 0x0008, \n\
		SUCC_TARGET_ARMED = 0x10, \n\
		SUCC_TARGET_DISARMED = 0x20 \n\
	}BIT_CODES_t; \n"
		<< std::endl
		<<
"	sTD_t* mTD; \n\
	std::vector<TDentry*> mEntryList; \n\
	sTD_t& get_struct(void){ return(*mTD); } \n\
	int push(TDentry *newEntry) { \n\
		for (auto const & it: mEntryList) { \n\
			if(it == newEntry) return (0); \n\
		} \n\
		mEntryList.push_back(newEntry); \n\
		return (1); \n\
	} \n"
		<< std::endl
		<<
"	int prep_inject(const char* targetname, const unsigned bit, const INJ_TYPE_t type = BITFLIP); \n\
	int prep_inject(const unsigned targetindex, const unsigned bit, const INJ_TYPE_t type = BITFLIP); \n\
	int reset_inject(const char* targetname); \n\
	int reset_inject(const unsigned targetindex); \n\
	void init(" << gen.mTopTypeName << "& pVRTL); \n\
	int get_EntryArrayIndex(const char* targetname) const; \n\
	int get_EntryArrayIndex(const unsigned targetindex) const; \n\
	TD_API(void): mTD(), mEntryList(){} \n\
};" <<	std::endl
		<< std::endl
		<<
"#endif /* __VRTLMODAPI_TARGETDICTIONARY_HPP__ */";

	body_ = x.str();
}

} // namespace vapi
