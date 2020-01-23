////////////////////////////////////////////////////////////////////////////////
/// @file apibuilder.hpp
/// @date Created on Mon Jan 07 14:12:11 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////


#ifndef INCLUDE_APIBUILD_APIBUILDER_HPP_
#define INCLUDE_APIBUILD_APIBUILDER_HPP_

#include "../APIbuild/xmlhelper.hpp"

////////////////////////////////////////////////////////////////////////////////
/// @class APIbuilder
/// @brief Handles generating the API from RegPicker-Xml and verilated model (VRTL)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class APIbuilder : public XmlHelper {
#define APIBUILDER_VERSION "0.9"
#define API_TD_DIRPREFIX "TD"
#define API_TD_HEADER_NAME "targetdictionary.hpp"
#define API_TD_SOURCE_NAME "targetdictionary.cpp"

	///////////////////////////////////////////////////////////////////////
	/// \brief Specified path to output directory
	const char* outdir;
public:
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns output directory
	std::string get_outputDir(void){return (std::string(outdir));}

	///////////////////////////////////////////////////////////////////////
	/// \brief Returns static to singleton instance
	static APIbuilder& _i(void){
		static APIbuilder _instance;
		return (_instance);
	}

	///////////////////////////////////////////////////////////////////////
	/// \brief Initiates instance
	/// \param pTargetXmlFile File path to RegPicker-Xml input
	/// \param pOutdir Directory path to where output is written
	int init(const char* pTargetXmlFile, const char* pOutdir);

	///////////////////////////////////////////////////////////////////////
	/// \brief Checks whether given Expr is a target in the Xml-Input
	/// \param pExpr Expression String
	/// \return -1 if not a target. >-1 as index in target list of XmlHelper
	int isExprTarget(const char* pExpr);

	///////////////////////////////////////////////////////////////////////
	/// \brief Get a Target of XmlHelper target list by index
	/// \param idx Index
	/// \return Reference to Target with index idx
	Target& getExprTarget(int idx);

	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing the include macros for API
	std::string getInludeStrings(void);

	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing the intermitten injection statement
	/// \param t Reference to Target
	std::string get_intermittenInjectionStmtString(Target& t);

	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing the sequential injection statement
	/// \param t Reference to Target
	/// \param word Default = -1 (trivial assignment). Otherwise word count in array assignment
	std::string get_sequentInjectionStmtString(Target& t, int word = -1);

	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing target dictionary name of a target
	/// \param t Reference to Target
	std::string get_targetdictionarytypedef(Target& t);

	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing target dictionary class definition of a target
	/// \param t Reference to Target
	std::string get_targetdictionaryEntryTypeDefString(Target& t);
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing target dictionary class declaration of a target
	/// \param t Reference to Target
	std::string get_targetdictionaryEntryDeclString(Target& t);

	///////////////////////////////////////////////////////////////////////
	/// \brief Build API: Target dictionary (.cpp/.hpp) and InjAPI to specified output directory
	int build_API(void);

protected:
	int build_targetdictionary(void);
	int build_targetdictionary_HPP(const char* outputdir);
	int build_targetdictionary_CPP(const char* outputdir);

	APIbuilder(void); //: mTargets(){};
	APIbuilder ( const APIbuilder& );
	APIbuilder & operator = (const APIbuilder &);
public:
	virtual ~APIbuilder(void){};
};


#endif /* INCLUDE_APIBUILD_APIBUILDER_HPP_ */
