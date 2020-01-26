////////////////////////////////////////////////////////////////////////////////
/// @file xmlhelper.cpp
/// @date Created on Mon Jan 10 19:54:03 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "APIbuild/xmlhelper.hpp"
#include "APIbuild/target.hpp"

#include "ftcv/Misc.h"

void XmlHelper::get_var(void) {
	static unsigned int targetcounter;

	const char *bits = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "bits");
	const char *name = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "name");
	const char *signalClass = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "signalClass");
	const char *type = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "type");
	const char *vhier = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "vHier");
	const char *cxxtype = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "CppType");

	sXmlEl x(name, vhier, signalClass, std::stoi(bits), type, cxxtype);
	Target *v = new Target(targetcounter, x);

	if (v->mElData.signalClass == sXmlEl::REG) {
		targetcounter++;
		mTargets.push_back(v);
		ftcv::log(ftcv::INFO, "XmlHelper: New Target found: ");
		ftcv::log(ftcv::INFO, v->_self());
	} else {
		delete (v);
	}
}

int XmlHelper::process_node(void) {
	std::string name = (const char*) xmlTextReaderConstName(mReader);

	if (name == "") {
		return (0);
	}
	if ((xmlTextReaderNodeType(mReader) == 1) and (xmlTextReaderHasAttributes(mReader) == 1)) { // node is an element and has attributes
		if ((name == "cell")) {
			std::string _name = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "name");
			std::string vHier = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "vHier");
			if ((_name == vHier) and (_name == "TOP")) {
				mTopTypeName = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "type");
				mTopName = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "name");
			}
		} else if ((name == "var") or (name == "out") or (name == "in")) { // node is a var or out
			get_var();
		}
	}
	return (1);
}
XmlHelper::XmlHelper(void) :
		mReader(), mTargets(), mFilepath(), mTopName(), mTopTypeName() {
}
XmlHelper::XmlHelper(const char *pXmlFile) :
		mReader(), mTargets(), mFilepath(pXmlFile), mTopName() {
	if (pXmlFile) {
		ftcv::log(ftcv::INFO, std::string("Register Xml-file found ") + pXmlFile);
		std::cout << "\t ... Start parsing" << std::endl;
		init(pXmlFile);
	}
}

XmlHelper::~XmlHelper(void) {
	if (mReader) {
		xmlFreeTextReader(mReader);
	}
}

int XmlHelper::init(const char *pXmlFile, int pOption, const char *encoding) {
	int ret = 1;
	if (pXmlFile == nullptr) {
		return (0);
	}
	mFilepath = pXmlFile;
	mReader = xmlReaderForFile(pXmlFile, encoding, pOption);
	if (mReader != NULL) {
		ret = xmlTextReaderRead(mReader);
		while (ret == 1) {
			process_node();
			ret = xmlTextReaderRead(mReader);
		}
		if (ret != 0) {
			ftcv::log(ftcv::ERROR, std::string("Error (XmlHelper): Failed to parse ") + pXmlFile);
		}
	} else {
		ftcv::log(ftcv::ERROR, std::string("Error (XmlHelper): Failed to open file ") + pXmlFile);
	}
	return ret;
}
