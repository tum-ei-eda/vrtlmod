#include "verilated.h"
#include "TD/targetdictionary.hpp"
#include "vrtlmod_api.hpp"

TDentry::TDentry(const char* name, unsigned index) :
	name(name), index(index), cntr(), enable(false), inj_type(INJ_TYPE::BITFLIP) {
//	api.push(this);
}
