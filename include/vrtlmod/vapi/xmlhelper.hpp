////////////////////////////////////////////////////////////////////////////////
/// @file xmlhelper.hpp
/// @date Created on Mon Jan 10 19:54:03 2020
/// @author Johannes Geier (johannes.geier@tum.de)
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_VAPI_XMLHELPER_HPP__
#define __VRTLMOD_VAPI_XMLHELPER_HPP__

#include <libxml/xmlreader.h>

#include "vrtlmod/vapi/target.hpp"

#include "vector"

namespace vapi {

////////////////////////////////////////////////////////////////////////////////
/// @class XmlHelper
/// @brief Simple Xml API class for RegPicker-Xml output
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class XmlHelper {
	///////////////////////////////////////////////////////////////////////
	/// \brief Pseudo callback for element extraction
	void get_var(std::string type);
protected:
	///////////////////////////////////////////////////////////////////////
	/// \brief Reader object
	xmlTextReaderPtr mReader;

public:
	///////////////////////////////////////////////////////////////////////
	/// \brief Vector containing all from Xml extracted targets
	std::vector<Target*> mTargets;

protected:
	///////////////////////////////////////////////////////////////////////
	/// \brief Path to input Xml file
	std::string mFilepath;
	///////////////////////////////////////////////////////////////////////
	/// \brief Name of VRTL top module
	std::string mTopName;
	///////////////////////////////////////////////////////////////////////
	/// \brief Type-Name of VRTL top module
	std::string mTopTypeName;
public:
	///////////////////////////////////////////////////////////////////////
	/// \brief Get extracted targets
	std::vector<Target*>& get_targets(void) {
		return (mTargets);
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Get xml input file path
	const char* get_xmlUri(void) {
		return (mFilepath.c_str());
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Process current Xml reader object node
	int process_node(void);
	///////////////////////////////////////////////////////////////////////
	/// \brief Initialize helper
	/// \param pXmlFile String containing the file path to input Xml
	/// \param pOption libxml2 parse option. Default 0
	/// \param encoding libxml2 encoding string. Default empty
	int init(const char *pXmlFile, int pOption = 0, const char *encoding = NULL);

	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor empty instance
	XmlHelper(void);
	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor self-initialization
	/// \param pXmlFile String containing the file path to input Xml
	XmlHelper(const char *pXmlFile);
	///////////////////////////////////////////////////////////////////////
	/// \brief Destructor
	virtual ~XmlHelper(void);
};

} // namespace vapi

#endif /* __VRTLMOD_VAPI_XMLHELPER_HPP__ */
