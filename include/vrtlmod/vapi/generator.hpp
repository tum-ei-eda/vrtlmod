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
/// @file generator.hpp
/// @date Created on Mon Jan 07 14:12:11 2020
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_VAPI_GENERATOR_HPP__
#define __VRTLMOD_VAPI_GENERATOR_HPP__

#include "vrtlmod/vrtlmod.hpp"

#include "vrtlmod/util/logging.hpp"
#include "vrtlmod/util/utility.hpp"

#include "vrtlmod/vapi/templates/templatefile.hpp"

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all core vrtlmod functionalities
namespace vrtlmod
{

class VrtlmodCore;

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all API building functionalities
namespace vapi
{

////////////////////////////////////////////////////////////////////////////////
/// @class VapiGenerator
/// @brief Handles generating the API from RegPicker-Xml and verilated model (VRTL)
class VapiGenerator
{

#define API_DIRPREFIX ""
#define API_TD_DIRPREFIX ""
#define API_TD_HEADER_NAME "targetdictionary.hpp"
#define API_HEADER_NAME "vrtlmodapi.hpp"
#define API_SOURCE_NAME "vrtlmodapi.cpp"

    class VapiSource final : public TemplateFile
    {
        VapiGenerator &gen_;

      public:
        VapiSource(VapiGenerator &gen) : gen_(gen) {}
        std::string get_brief(void) const { return "vrtlmod api source"; }
        std::string get_details(void) const { return "automatically generated file"; }
        std::string get_author(void) const { return util::concat("vrtlmod::vapi::VapiSource v", get_version()); }
        std::string generate_body(void) const;
    } vapisrc_{ *this };

    class VapiHeader final : public TemplateFile
    {
        VapiGenerator &gen_;

      public:
        VapiHeader(VapiGenerator &gen) : gen_(gen) {}
        std::string get_brief(void) const { return "vrtlmod api header"; }
        std::string get_details(void) const { return "automatically generated file"; }
        std::string get_author(void) const { return util::concat("vrtlmod::vapi::VapiHeader v", get_version()); }
        std::string generate_body(void) const;
    } vapiheader_{ *this };

    class TDHeader final : public TemplateFile
    {
        VapiGenerator &gen_;

      public:
        TDHeader(VapiGenerator &gen) : gen_(gen) {}
        std::string get_brief(void) const { return "vrtlmod target dictionary header"; }
        std::string get_details(void) const
        {
            return "automatically generated file for standalone vrtlmod executable, sourced by "
                   "$VRTLMOD_ROOT/include/targetdictionary.hpp";
        }
        std::string get_author(void) const { return util::concat("vrtlmod::vapi::TDHeader v", get_version()); }
        std::string generate_header(std::string filename) const;
        std::string generate_body(void) const;
    } tdheader_{ *this };

    std::vector<std::string> prepare_files(const std::vector<std::string> &files,
                                           const std::vector<std::string> &file_ext_matchers, bool overwrite);
    const VrtlmodCore &core_;

  public:
    const VrtlmodCore &get_core() const { return core_; }

  public:
    std::string get_targetdictionary_filename(void) const { return (API_TD_HEADER_NAME); }
    std::string get_targetdictionary_relpath(void) const;
    std::string get_apiheader_filename(void) const;
    std::string get_apisource_filename(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Returns String containing the include macros for API
    std::string getInludeStrings(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Build API: vrtlmod (.cpp/.hpp) and InjAPI to specified output directory
    int build_api(void) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Build API: Target dictionary (targetdictionary.hpp)
    int build_targetdictionary(void) const;
    std::string get_td_string(void) const
    {
        return tdheader_.generate_header(API_TD_HEADER_NAME) + tdheader_.generate_body();
    }

  public:
    ///////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    VapiGenerator(VrtlmodCore &vrtlmod_core);

  public:
    virtual ~VapiGenerator(void){};
};

} // namespace vapi
} // namespace vrtlmod

#endif /* __VRTLMOD_VAPI_GENERATOR_HPP__ */
