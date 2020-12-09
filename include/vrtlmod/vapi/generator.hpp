////////////////////////////////////////////////////////////////////////////////
/// @file generator.hpp
/// @date Created on Mon Jan 07 14:12:11 2020
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_VAPI_GENERATOR_HPP__
#define __VRTLMOD_VAPI_GENERATOR_HPP__

#include "vrtlmod/vapi/xmlhelper.hpp"
#include "vrtlmod/vapi/templates/templatefile.hpp"

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all API building functionalities
namespace vapi {

////////////////////////////////////////////////////////////////////////////////
/// @class VapiGenerator
/// @brief Handles generating the API from RegPicker-Xml and verilated model (VRTL)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class VapiGenerator: public XmlHelper {
public:
#define APIBUILDER_VERSION "0.9"
#define API_DIRPREFIX "vrtlmod"
#define API_TD_DIRPREFIX "targetdictionary"
#define API_TD_HEADER_NAME "targetdictionary.hpp"
#define API_HEADER_NAME "vrtlmodapi.hpp"
#define API_SOURCE_NAME "vrtlmodapi.cpp"
private:
	///////////////////////////////////////////////////////////////////////
	/// \brief Specified path to output directory
	const char *outdir;

	class TargetDictionary final : public TemplateFile {
		public:
			TargetDictionary(void) {}
			const std::string get_brief(void){return (std::string("vrtlmod target dictionary"));}
			const std::string get_details(void){return (std::string("automatically generated file"));}
			const std::string get_author(void){return (std::string("vrtlmod::vapi::VapiGenerator v0.9"));}
			void generate_body(void);
	} td_;

	class VapiSource final : public TemplateFile {
		public:
			VapiSource(void) {}
			const std::string get_brief(void){return (std::string("vrtlmod api source"));}
			const std::string get_details(void){return (std::string("automatically generated file"));}
			const std::string get_author(void){return (std::string("vrtlmod::vapi::VapiSource v0.9"));}
			void generate_body(void);
	} vapisrc_;

	class VapiHeader final : public TemplateFile {
		public:
			VapiHeader(void) {}
			const std::string get_brief(void){return (std::string("vrtlmod api header"));}
			const std::string get_details(void){return (std::string("automatically generated file"));}
			const std::string get_author(void){return (std::string("vrtlmod::vapi::VapiHeader v0.9"));}
			void generate_body(void);
	} vapiheader_;

public:

	///////////////////////////////////////////////////////////////////////
	/// \brief Returns output directory
	std::string get_outputDir(void) {
		return (std::string(outdir));
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns static to singleton instance
	static VapiGenerator& _i(void) {
		static VapiGenerator _instance;
		return (_instance);
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Initiates instance
	/// \param pTargetXmlFile File path to RegPicker-Xml input
	/// \param pOutdir Directory path to where output is written
	int init(const char *pTargetXmlFile, const char *pOutdir);
	///////////////////////////////////////////////////////////////////////
	/// \brief Checks whether given Expr is a target in the Xml-Input
	/// \param pExpr Expression String
	/// \return -1 if not a target. >-1 as index in target list of XmlHelper
	int isExprTarget(const char *pExpr);
	///////////////////////////////////////////////////////////////////////
	/// \brief Get a Target of XmlHelper target list by index
	/// \param idx Index
	/// \return Reference to Target with index idx
	Target& getExprTarget(int idx);
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing the include macros for API
	std::string getInludeStrings(void);
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing the (external) declaration of target dictionary (once full decl)
	std::string getTDExternalDecl(void);
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing the intermitten injection statement
	/// \param t Reference to Target
	std::string get_intermittenInjectionStmtString(Target &t);
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing the sequential injection statement
	/// \param t Reference to Target
	/// \param word Default = -1 (trivial assignment). Otherwise word count in array assignment
	std::string get_sequentInjectionStmtString(Target &t, int word = -1);
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing target dictionary definition name of a target (class definition)
	/// \param t Reference to Target
	std::string get_targetdictionaryTargetClassDefName(Target &t);
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing target dictionary declaration name of a target (class instance)
	/// \param t Reference to Target
	std::string get_targetdictionaryTargetClassDeclName(Target &t);
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing target dictionary class definition of a target
	/// \param t Reference to Target
	std::string get_targetdictionaryEntryTypeDefString(Target &t);
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns String containing target dictionary class declaration of a target
	/// \param t Reference to Target
	std::string get_targetdictionaryEntryDeclString(Target &t);
	///////////////////////////////////////////////////////////////////////
	/// \brief Build API: Target dictionary (.cpp/.hpp) and InjAPI to specified output directory
	int build_API(void);
protected:
	int build_targetdictionary(void);
	int build_targetdictionary_HPP(const char *outputdir);
	int build_API_HPP(const char *outputdir);
	int build_API_CPP(const char *outputdir);
	std::string get_fileheader(const char *filename);
	VapiGenerator(void); //: mTargets(){};
	VapiGenerator(const VapiGenerator&);
	VapiGenerator& operator =(const VapiGenerator&);
public:
	virtual ~VapiGenerator(void) {
	}
	;
};

} // namespace vapi

#endif /* __VRTLMOD_VAPI_GENERATOR_HPP__ */
