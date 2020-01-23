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
	const char* outdir;
public:
	std::string get_outputDir(void){return (std::string(outdir));}

	static APIbuilder& _i(void){
		static APIbuilder _instance;
		return (_instance);
	}

	int init(const char* pTargetXmlFile, const char* pOutdir);
	int isExprTarget(const char* pExpr);
	Target& getExprTarget(int idx);

	std::string getInludeStrings(void);

	std::string get_intermittenInjectionStmtString(Target& t);
	std::string get_sequentInjectionStmtString(Target& t, int word = -1);

	std::string get_targetdictionarytypedef(Target& t);

	std::string get_targetdictionaryEntryTypeDefString(Target& t);
	std::string get_targetdictionaryEntryDeclString(Target& t);

	int build_API(void);

	int build_targetdictionary(void);
protected:
	int build_targetdictionary_HPP(const char* outputdir);
	int build_targetdictionary_CPP(const char* outputdir);

	APIbuilder(void); //: mTargets(){};
	APIbuilder ( const APIbuilder& );
	APIbuilder & operator = (const APIbuilder &);
public:
	virtual ~APIbuilder(void){};

};


#endif /* INCLUDE_APIBUILD_APIBUILDER_HPP_ */
