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

namespace vapi
{

////////////////////////////////////////////////////////////////////////////////
/// @class XmlHelper
/// @brief Simple Xml API class for RegPicker-Xml output
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class XmlHelper
{
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
    std::vector<Target *> mTargets;

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
    std::vector<Target *> &get_targets(void) { return (mTargets); }
    ///////////////////////////////////////////////////////////////////////
    /// \brief Get xml input file path
    const char *get_xmlUri(void) { return (mFilepath.c_str()); }
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
