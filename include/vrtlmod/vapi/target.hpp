/*
 * Copyright 2021 Chair of EDA, Technical University of Munich
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

////////////////////////////////////////////////////////////////////////////////
/// @file target.hpp
/// @date Created on Mon Jan 10 18:21:20 2020
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_VAPI_TARGET_HPP__
#define __VRTLMOD_VAPI_TARGET_HPP__

#include <string>
#include <sstream>
#include <algorithm>

#include "vrtlmod/util/logging.hpp"

#include <boost/lexical_cast.hpp>

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all API building functionalities
namespace vapi {
class VapiGenerator;
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
	/// \brief Element's total number of bits
	unsigned int nmbBits;
	///////////////////////////////////////////////////////////////////////
	/// \brief Element's one-dimensional, i.e., element, number of bits
	unsigned int nmbonedimBits;
	///////////////////////////////////////////////////////////////////////
	/// \brief Element's dimensions of one-dimensional bit vectors
	std::vector<int> dim_{};
	
	std::string cxxbasetype_;
	std::vector<int> cxxdim_{};
	std::vector<std::string> cxxtypedim_{};
	///////////////////////////////////////////////////////////////////////
	/// \brief Element's VRTL type
	//std::string type;
	///////////////////////////////////////////////////////////////////////
	/// \brief Number of words
//	unsigned int words;
	///////////////////////////////////////////////////////////////////////
	/// \brief Element's VRTL c++ type (CData, IData, WData[..], ...)
	std::string vrtlCxxType;
	///////////////////////////////////////////////////////////////////////
	/// \brief Copy Constructor
	sXmlEl(const sXmlEl &x) 
		: name(x.name)
		, hierarchy(x.hierarchy)
		, signalClass(x.signalClass)
		, nmbBits(x.nmbBits)
		, nmbonedimBits(x.nmbonedimBits)
		, dim_(x.dim_)
		, cxxbasetype_(x.cxxbasetype_)
		, cxxdim_(x.cxxdim_)
		, cxxtypedim_(x.cxxtypedim_)
		//, type(x.type)
		, vrtlCxxType(x.vrtlCxxType) {
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor
	sXmlEl(const char *name = "", const char *hierarchy = "", const char *signalClass_s = "", unsigned int nmbBits = 0, const char* dim = "", const char* vrtlCxxType = "", const char* bases = "") 
		: name(name)
		, hierarchy(hierarchy)
		, signalClass(UNDEF)
		, nmbBits(nmbBits)
		, nmbonedimBits(0)
		, dim_()
		, cxxbasetype_(vrtlCxxType)
		, cxxdim_()
		//, type(type)
		, vrtlCxxType(vrtlCxxType) 
	{
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
		std::string s, dimstr = dim;
		util::logging::log(util::logging::INFO, dimstr);
		auto brOpen = dimstr.find('[');
		auto brClose = dimstr.rfind(']');
		dimstr = dimstr.substr(brOpen+1, brClose - brOpen -1);
		dimstr.erase(remove_if(dimstr.begin(), dimstr.end(), isspace), dimstr.end());
		std::istringstream x(
			dimstr
		);
		while(getline(x, s, ',')) {
			util::logging::log(util::logging::INFO, s);
			s = s.substr(s.find("'")+1, s.rfind("'")-s.find("'")-1);
			util::logging::log(util::logging::INFO, s);
			if( s.find(':') == std::string::npos )
				dim_.push_back(boost::lexical_cast<int>(s));
			else{
				std::string up = s.substr(0, s.find(':'));
				std::string to = s.substr(s.find(':')+1);
				int iup = boost::lexical_cast<int>(up);
				int ito = boost::lexical_cast<int>(to);
				nmbonedimBits = iup-ito+1;
				dim_.push_back(nmbonedimBits);
			}
		}

		std::string basetypestr = bases;
		util::logging::log(util::logging::INFO, std::string("*")+vrtlCxxType+std::string(":=")+bases);
		brOpen = basetypestr.find('[');
		brClose = basetypestr.rfind(']');
		basetypestr = basetypestr.substr(brOpen+1, brClose - brOpen -1);
		basetypestr.erase(remove_if(dimstr.begin(), dimstr.end(), isspace), dimstr.end());
		std::istringstream basetypestream(
			basetypestr
		);
		while(getline(basetypestream, s, ',')) {
			util::logging::log(util::logging::INFO, s);
			s = s.substr(s.find("'")+1, s.rfind("'")-s.find("'")-1);
			util::logging::log(util::logging::INFO, s);
			cxxtypedim_.push_back(s);
		}
		
		cxxdim_ = dim_;
		if(cxxtypedim_.back().find("VlWide<") != std::string::npos){
			dimstr = cxxtypedim_.back();
			brOpen = dimstr.find('<');
			brClose = dimstr.rfind('>');
			dimstr = dimstr.substr(brOpen+1, brClose - brOpen -1);
			cxxdim_.back() = boost::lexical_cast<int>(dimstr);
			cxxtypedim_.push_back("EData");
		} else { //remove last cxxdim_ element (numberof1dimbits)
			cxxdim_.pop_back();
		}
	}
} sXmlEl_t;

class ExprT {
public:
	std::string expr;
	std::string prefix;
	std::string object;
	std::string name;
	ExprT(const char *Expr);
};

////////////////////////////////////////////////////////////////////////////////
/// @class Target
/// @brief Corresponds to RegPicker-Xml element information
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class Target {
	friend VapiGenerator;
public:
	///////////////////////////////////////////////////////////////////////
	/// \brief Number of times the target received a SEQ_INJ
	unsigned mSeqInjCnt;
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
	std::string get_hierarchy(void);
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns the targets undotted hierarchy ("__DOT__"s instead of "."s)
	std::string get_hierarchyDedotted(void);
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
		os << *obj;	//->_self();
		return os;
	}
};

} // namespace vapi

#endif /* __VRTLMOD_VAPI_TARGET_HPP__ */
