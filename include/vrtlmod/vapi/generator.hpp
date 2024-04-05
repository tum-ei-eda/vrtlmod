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
#define API_DIFF_HEADER_NAME "vrtlmod_diffapi.hpp"
#define API_DIFF_SOURCE_NAME "vrtlmod_diffapi.cpp"
#define API_DIFF_COMPARE_FAST_SOURCE_NAME "vrtlmod_diffapi_compare_fast.cpp"
#define API_DIFF_COMPARE_SOURCE_NAME "vrtlmod_diffapi_compare.cpp"
#define API_DIFF_COMPUTE_SOURCE_NAME "vrtlmod_diffapi_compute.cpp"
#define API_PYTHON_TD_NAME "vrtlmod_td_module.py"

    struct VapiSource final : public TemplateFile
    {
        VapiGenerator &gen_;
        VapiSource(VapiGenerator &gen) : gen_(gen) {}
        std::string get_brief(void) const { return "vrtlmod api source"; }
        std::string get_details(void) const { return "automatically generated file"; }
        std::string get_author(void) const { return util::concat("vrtlmod::vapi::VapiSource v", get_version()); }
        std::string generate_body(void) const;
    } vapisrc_{ *this };

    struct VapiHeader final : public TemplateFile
    {
        VapiGenerator &gen_;
        VapiHeader(VapiGenerator &gen) : gen_(gen) {}
        std::string get_brief(void) const { return "vrtlmod api header"; }
        std::string get_details(void) const { return "automatically generated file"; }
        std::string get_author(void) const { return util::concat("vrtlmod::vapi::VapiHeader v", get_version()); }
        std::string generate_body(void) const;
    } vapiheader_{ *this };

    struct TDHeader final : public TemplateFile
    {
        VapiGenerator &gen_;

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

    struct VapiDiffSource final : public TemplateFile
    {
        VapiGenerator &gen_;
        VapiDiffSource(VapiGenerator &gen) : gen_(gen) {}
        std::string get_brief(void) const { return "vrtlmod api diff source"; }
        std::string get_details(void) const { return "automatically generated file"; }
        std::string get_author(void) const { return util::concat("vrtlmod::vapi::VapiDiffSource v", get_version()); }
        std::string generate_body(void) const;
        std::string get_filename(void) const;
    } vapi_diff_cpp_{ *this };

    struct VapiDiffHeader final : public TemplateFile
    {
        VapiGenerator &gen_;
        VapiDiffHeader(VapiGenerator &gen) : gen_(gen) {}
        std::string get_brief(void) const { return "vrtlmod api diff module header"; }
        std::string get_details(void) const { return "automatically generated file"; }
        std::string get_author(void) const { return util::concat("vrtlmod::vapi::VapiDiffHeader v", get_version()); }
        std::string generate_body(void) const;
        std::string get_filename(void) const;
    } vapi_diff_hpp_{ *this };

    struct VapiDiffCompareFastSource final : public TemplateFile
    {
        VapiGenerator &gen_;
        VapiDiffCompareFastSource(VapiGenerator &gen) : gen_(gen) {}
        std::string get_brief(void) const { return "vrtlmod api diff compare source"; }
        std::string get_details(void) const { return "automatically generated file"; }
        std::string get_author(void) const { return util::concat("vrtlmod::vapi::VapiDiffCompareFastSource v", get_version()); }
        std::string generate_body(void) const;
        std::string get_filename(void) const;
    } vapi_diff_compare_fast_cpp_{ *this };

    struct VapiDiffCompareSource final : public TemplateFile
    {
        VapiGenerator &gen_;
        VapiDiffCompareSource(VapiGenerator &gen) : gen_(gen) {}
        std::string get_brief(void) const { return "vrtlmod api diff compare source"; }
        std::string get_details(void) const { return "automatically generated file"; }
        std::string get_author(void) const { return util::concat("vrtlmod::vapi::VapiDiffCompareSource v", get_version()); }
        std::string generate_body(void) const;
        std::string get_filename(void) const;
    } vapi_diff_compare_cpp_{ *this };

    struct VapiDiffComputeSource final : public TemplateFile
    {
        VapiGenerator &gen_;
        VapiDiffComputeSource(VapiGenerator &gen) : gen_(gen) {}
        std::string get_brief(void) const { return "vrtlmod api diff compute source"; }
        std::string get_details(void) const { return "automatically generated file"; }
        std::string get_author(void) const { return util::concat("vrtlmod::vapi::VapiDiffComputeSource v", get_version()); }
        std::string generate_body(void) const;
        std::string get_filename(void) const;
    } vapi_diff_compute_cpp_{ *this };

    struct VapiTargetDictionaryPythonModule final: public TemplateFile
    {
        VapiGenerator &gen_;
        VapiTargetDictionaryPythonModule(VapiGenerator &gen) : gen_(gen) {}
        std::string get_brief(void) const { return "vrtlmod api target dictionary python module"; }
        std::string get_details(void) const { return "automatically generated file"; }
        std::string get_author(void) const { return util::concat("vrtlmod::vapi::VapiTargetDictionaryPythonModule v", get_version()); }
        std::string generate_body(void) const;
        std::string get_filename(void) const;
        std::string generate_header(std::string filename) const override;
    } vapi_td_python_module_{ *this };

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
    std::string get_diffapiheader_filename(void) const;
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
