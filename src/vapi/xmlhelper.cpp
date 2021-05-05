////////////////////////////////////////////////////////////////////////////////
/// @file xmlhelper.cpp
/// @date Created on Mon Jan 10 19:54:03 2020
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vapi/xmlhelper.hpp"
#include "vrtlmod/vapi/target.hpp"
#include "vrtlmod/util/logging.hpp"

#include <boost/lexical_cast.hpp>

namespace vapi {

void XmlHelper::get_var(std::string type) {

	if (type != "var") return;
	
	static unsigned int targetcounter;

	const char *signalClass = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "sigclass");
	const char *name = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "name");
	const char *bits = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "bits");
	const char *bases = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "bases");
	const char *dim = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "dim");
	const char *vhier = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "vhier");
	const char *cxxtype = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "cxxtype");

	// const char *onedimbits = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "onedimbits");
	// const char *type = (const char*) xmlTextReaderGetAttribute(mReader, (const xmlChar*) "type");

	int _bits = 0, _onedimbits = 0;
	try {
		_bits = boost::lexical_cast<int>(bits);
	} catch (boost::bad_lexical_cast) {
		util::logging::log(util::logging::ERROR, std::string("Could not convert to integer:\tbits=") + std::string(bits) );
	}	
		
	sXmlEl x(name, vhier, signalClass, _bits, dim, cxxtype, bases);
	Target *v = new Target(targetcounter, x);

	if (v->mElData.signalClass == sXmlEl::REG) {
		targetcounter++;
		mTargets.push_back(v);
		util::logging::log(util::logging::INFO, "XmlHelper: New Target found: ");
		util::logging::log(util::logging::INFO, v->_self());
	} else if(v->mElData.signalClass == sXmlEl::UNDEF) {
		targetcounter++;
		mTargets.push_back(v);
		util::logging::log(util::logging::INFO, "XmlHelper: UNDEF type signal found. Will try to provide injection point: ");
		util::logging::log(util::logging::INFO, v->_self());
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
			get_var(name);
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
		util::logging::log(util::logging::INFO, std::string("Register Xml-file found ") + pXmlFile);
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
			util::logging::log(util::logging::ERROR, std::string("Error (XmlHelper): Failed to parse ") + pXmlFile);
		}
	} else {
		util::logging::log(util::logging::ERROR, std::string("Error (XmlHelper): Failed to open file ") + pXmlFile);
	}
	return ret;
}

} // namespace vapi
