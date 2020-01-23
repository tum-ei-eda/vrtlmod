////////////////////////////////////////////////////////////////////////////////
/// @file xmlhelper.hpp
/// @date Created on Mon Jan 10 19:54:03 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_APIBUILD_XMLHELPER_HPP_
#define INCLUDE_APIBUILD_XMLHELPER_HPP_

#include "libxml/xmlreader.h"
#include "vector"

#include "../APIbuild/target.hpp"


////////////////////////////////////////////////////////////////////////////////
/// @class XmlHelper
/// @brief Simple Xml API class for RegPicker-Xml output
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class XmlHelper {

protected:
	xmlTextReaderPtr mReader;
public:
	std::vector<Target*> mTargets;
protected:
	std::string mFilepath;
	std::string mTopName;
public:
	std::vector<Target*>& get_targets(void) {return (mTargets);}
	const char* get_xmlUri(void){return (mFilepath.c_str());}
private:
	void get_var(void);

public:
	int process_node(void);
	int init(const char *pXmlFile, int pOption = 0, const char *encoding = NULL);

	XmlHelper(void);
	XmlHelper(const char *pXmlFile);
	virtual ~XmlHelper(void);
};

#endif /* INCLUDE_APIBUILD_XMLHELPER_HPP_ */
