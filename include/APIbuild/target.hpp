////////////////////////////////////////////////////////////////////////////////
/// @file target.hpp
/// @date Created on Mon Jan 10 18:21:20 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_APIBUILD_TARGET_HPP_
#define INCLUDE_APIBUILD_TARGET_HPP_

#include <string>
#include <iostream>

class APIbuilder;

typedef struct sXmlEl {
	typedef enum {
		REG = 1, CONST = 2, WIRE = 3, UNDEF = 0
	} signalClass_t;

	std::string name;
	std::string hierarchy;

	signalClass_t signalClass;
	unsigned int nmbBits;
	std::string type;
	std::string vrtlCxxType;

	sXmlEl(const sXmlEl &x) : name(x.name), hierarchy(x.hierarchy), signalClass(x.signalClass), nmbBits(x.nmbBits), type(x.type), vrtlCxxType(
			x.vrtlCxxType) {
	}
	sXmlEl(const char *name = "", const char *hierarchy = "", const char *signalClass_s = "", unsigned int nmbBits = 0, const char *type =
			"", const char *vrtlCxxType = "") : name(name), hierarchy(hierarchy), signalClass(UNDEF), nmbBits(nmbBits), type(type), vrtlCxxType(
			vrtlCxxType) {
		if (signalClass_s != NULL) {
			std::string x = signalClass_s;
			if (x == "register") {
				signalClass = REG;
			} else if (x == "wire") {
				signalClass = WIRE;
			} else if (x == "constant") {
				signalClass = CONST;
			} else {
				signalClass = UNDEF;
			}
		}
	}
} sXmlEl_t;

////////////////////////////////////////////////////////////////////////////////
/// @class Target
/// @brief Corresponds to RegPicker-Xml element information
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class Target {
protected:
	unsigned int index;
public:
	friend APIbuilder;

	sXmlEl_t mElData;

	std::string mTD_typedef;
	std::string mTD_decl;

	std::string get_hierarchy(void) {
		std::string ret = mElData.hierarchy.substr(mElData.hierarchy.find(".") + 1);
		return (ret);
	}

	std::string get_hierarchyDedotted(void) {
		std::string ret = get_hierarchy();
		if (ret.find(".") != std::string::npos) {
			ret = ret.replace(ret.begin(), ret.end(), ".", "__DOT__");
		}
		return (ret);
	}

	unsigned int get_index(void) const {
		return (index);
	}

	std::string _self(void) const;

	Target(unsigned int index, sXmlEl_t &data);

	virtual ~Target(void) {
	}

	friend std::ostream& operator<<(std::ostream &os, const Target &obj) {
		os << obj._self();
		return os;
	}
	friend std::ostream& operator<<(std::ostream &os, const Target *obj) {
		os << obj->_self();
		return os;
	}
};

#endif /* INCLUDE_APIBUILD_TARGET_HPP_ */
