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

////////////////////////////////////////////////////////////////////////////////
/// @struct sXmlEl
/// @brief C++ struct equivalent for RegPicker's Xml elements
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
typedef struct sXmlEl {
	typedef enum {
		REG = 1, CONST = 2, WIRE = 3, UNDEF = 0
	} signalClass_t;
	///////////////////////////////////////////////////////////////////////
	/// \brief Element's name
	std::string name;
	///////////////////////////////////////////////////////////////////////
	/// \brief Element's VRTL hierarchy
	std::string hierarchy;
	///////////////////////////////////////////////////////////////////////
	/// \brief Element's signal class
	signalClass_t signalClass;
	///////////////////////////////////////////////////////////////////////
	/// \brief Element's number of bits
	unsigned int nmbBits;
	///////////////////////////////////////////////////////////////////////
	/// \brief Element's VRTL type
	std::string type;
	///////////////////////////////////////////////////////////////////////
	/// \brief Element's VRTL c++ type (CData, IData, WData[..], ...)
	std::string vrtlCxxType;
	///////////////////////////////////////////////////////////////////////
	/// \brief Copy Constructor
	sXmlEl(const sXmlEl &x) : name(x.name), hierarchy(x.hierarchy), signalClass(x.signalClass), nmbBits(x.nmbBits), type(x.type), vrtlCxxType(
			x.vrtlCxxType) {
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor
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
	friend APIbuilder;
protected:
	///////////////////////////////////////////////////////////////////////
	/// \brief Unique index of target
	unsigned int index;
public:
	///////////////////////////////////////////////////////////////////////
	/// \brief Xml data
	sXmlEl_t mElData;
	///////////////////////////////////////////////////////////////////////
	/// \brief String containing the target's API target dictionary definition string
	std::string mTD_typedef;
	///////////////////////////////////////////////////////////////////////
	/// \brief String containing the target's API target dictionary declaration string
	std::string mTD_decl;
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns the targets hierarchy
	std::string get_hierarchy(void) {
		std::string ret = mElData.hierarchy.substr(mElData.hierarchy.find(".") + 1);
		return (ret);
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns the targets undotted hierarchy ("__DOT__"s instead of "."s)
	std::string get_hierarchyDedotted(void) {
		std::string ret = get_hierarchy();
		if (ret.find(".") != std::string::npos) {
			ret = ret.replace(ret.begin(), ret.end(), ".", "__DOT__");
		}
		return (ret);
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns the targets index
	unsigned int get_index(void) const {
		return (index);
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Self representation of this target
	std::string _self(void) const;
	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor
	/// \param index
	/// \param data
	Target(unsigned int index, sXmlEl_t &data);
	///////////////////////////////////////////////////////////////////////
	/// \brief Destructor
	virtual ~Target(void) {
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Overloaded operator for stream access of class (reference)
	friend std::ostream& operator<<(std::ostream &os, const Target &obj) {
		os << obj._self();
		return os;
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Overloaded operator for stream access of class (pointer)
	friend std::ostream& operator<<(std::ostream &os, const Target *obj) {
		os << *obj;//->_self();
		return os;
	}
};

#endif /* INCLUDE_APIBUILD_TARGET_HPP_ */
