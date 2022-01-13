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
/// \file parser.cpp
/// \date Created on Mon Dec 8 20:35:03 2020
/// \author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vxml/parser.hpp"
#include "vrtlmod/util/logging.h"

namespace vxml {

int VxmlParser::process_node(void) {
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

VxmlParser::VxmlParser(void) :
	mReader(), mFilepath(), mTopName(), mTopTypeName() {
}

VxmlParser::VxmlParser(const char *pXmlFile) :
	mReader(), mTargets(), mFilepath(pXmlFile), mTopName() {
	if (pXmlFile) {
		util::logging::log(util::logging::INFO, std::string("Register Xml-file found ") + pXmlFile);
		std::cout << "\t ... Start parsing" << std::endl;
		init(pXmlFile);
	}
}

VxmlParser::~VxmlParser(void) {
	if (mReader) {
		xmlFreeTextReader(mReader);
	}
}

int VxmlParser::init(const char *pXmlFile, int pOption, const char *encoding) {
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
} // namespace vxml
