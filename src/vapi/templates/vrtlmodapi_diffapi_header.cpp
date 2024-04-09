/*
 * Copyright 2024 Chair of EDA, Technical University of Munich
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

#include "vrtlmod/vapi/generator.hpp"
#include "vrtlmod/core/core.hpp"
#include "vrtlmod/core/types.hpp"

namespace vrtlmod
{
namespace vapi
{

std::string VapiGenerator::VapiDiffHeader::get_filename(void) const
{
    std::string top_name = gen_.get_core().get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4204
    // nothing to do here
#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif

    return util::concat(top_name, "_", API_DIFF_HEADER_NAME);
}

std::string VapiGenerator::VapiDiffHeader::generate_body(void) const
{
    const auto &core = gen_.get_core();

    std::string top_name = core.get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4202
#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif
    std::string top_type = top_name;
    std::stringstream x, entries;

    std::string api_name = top_type + "VRTLmodAPI";

    x << R"(#ifndef __)" << top_type << R"(_VRTLMOD_DIFFAPI_HPP__
#define __)"
      << top_type << R"(_VRTLMOD_DIFFAPI_HPP__
)"
      << R"(
#include ")"
      << gen_.get_targetdictionary_relpath() << R"("
#include ")"
      << top_type << R"(_vrtlmodapi.hpp"
#include <iostream>
#include <list>

)";

      x << R"(
struct )"

      << api_name << "Differential : public " << api_name << R"(
{
)";
    if (core.is_systemc())
    {
        if (auto top_module = core.get_module_from_cell(core.get_top_cell()))
        {
            for (auto const &var : top_module->variables_)
            {
                std::string name = var->get_type();
                if (name == "in" || name == "out" || name == "inout")
                {
                    auto type = var->get_bases();
                    util::strhelp::replace(type, "[", "");
                    util::strhelp::replace(type, "]", "");
                    auto base_type = type;

                    auto port_name = var->get_id();
                    // replace the systemc port type with a systemc signal type equivalent
                    if (!util::strhelp::replace(type, "sc_out<", "sc_signal<"))
                        if (!util::strhelp::replace(type, "sc_in<", "sc_signal<"))
                            util::strhelp::replace(type, "sc_inout<", "sc_signal<");

                    // remvove the systemc port type from the signal's type
                    if (!util::strhelp::replace(base_type, "sc_out<", ""))
                        if (!util::strhelp::replace(base_type, "sc_in<", ""))
                            util::strhelp::replace(base_type, "sc_inout<", "");
                    util::strhelp::replace(base_type, ">", "");

                            x
                        << R"(
    )" << type << " " << port_name
                        << "_dummy_{\"dummy_" << port_name << "\"};";
                    x << R"(
    )" << base_type << " "
                      << port_name << "_diff_;"; //{\"diff_" << port_name << "\"};";
                }
            }
        }
    }

    x << R"(
    const )"
      << api_name << "& faulty_; ///< Fault injection core"
      << R"(
    const )"
      << api_name << "& reference_; ///< Reference core"
      << R"(

    /////////////////////////////////////////////////////////////////////////////
    /// \brief Get faulty target entry for passed id
    /// \param id passed target dictionary. Take care id for a target may change between vRTLmod of the same vRTL
    vrtlfi::td::TDentry const *get_faulty_target(size_t id) const { return faulty_.id2target_.at(id); }
    /////////////////////////////////////////////////////////////////////////////
    /// \brief Get reference target entry for passed id
    /// \param id passed target dictionary. Take care id for a target may change between vRTLmod of the same vRTL
    vrtlfi::td::TDentry const *get_reference_target(size_t id) const { return reference_.id2target_.at(id); }
    /////////////////////////////////////////////////////////////////////////////
    /// \brief Get diff target entry for passed id
    /// \param id passed target dictionary. Take care id for a target may change between vRTLmod of the same vRTL
    vrtlfi::td::TDentry const *get_diff_target(size_t id) const { return this->id2target_.at(id); }
    /////////////////////////////////////////////////////////////////////////////
    /// \brief Get target entry for passed id
    /// \param id passed target dictionary. Take care id for a target may change between vRTLmod of the same vRTL
    vrtlfi::td::TDentry const *get_target(size_t id) const { return get_faulty_target(id); }
    /////////////////////////////////////////////////////////////////////////////
    /// \brief Get link id for passed
    /// \param target target which can be a pointer to an element of either
    ///        of either `faulty_`, `reference_`, or `this`
    size_t get_id(vrtlfi::td::TDentry const *target) const;
)";

    x << R"(

    /////////////////////////////////////////////////////////////////////////////
    /// \brief Calculate diff between `faulty_` and `reference_` target dict-
    ///        ionaries. Store diff (bitwise XOR) in )"
      << api_name << R"( base
    /// \return count of mismatching targets
    int diff_target_dictionaries(void);

    /////////////////////////////////////////////////////////////////////////////
    /// \brief Compare `faulty_` with `reference_`.
    /// \param start Begin diff loop with target default or nullptr starts at
    ///              list entry..
    /// \return First mismatching target `vrtlfi::td::TDentry`.
    vrtlfi::td::TDentry const* compare_fast(vrtlfi::td::TDentry const *start = nullptr) const;

    /////////////////////////////////////////////////////////////////////////////
    /// \brief Diff a single target with a given value.
    /// \param target_idx Target index
    /// \param element_idx Element index of the given target (serialized multidim
    ///                    access)
    /// \param val value to compare with, will be masked with the element's bit
    ///            mask
    /// \return True on diff=0 False diff!=0
    bool diff_target(size_t target_id, size_t element_id, uint64_t val) const;

    /////////////////////////////////////////////////////////////////////////////
    /// \brief Diff a single target with a given value.
    /// \param diff_in tuple containing a set of indices for target and element
    ///                and the value to diff with
    /// \return True on diff=0 False diff!=0
    bool diff_target(vrtlfi::td::UniqueElementTriplet const& diff_in) const {
        return diff_target(diff_in.target_id_, diff_in.element_id_, diff_in.val_);
    }

    /////////////////////////////////////////////////////////////////////////////
    /// \brief Computes the diff and returns a vector of diff triplets. Compute 
    ///        store is done on the DIFF-API's own states. Empty return means no
    ///        diff.
    std::vector<vrtlfi::td::UniqueElementTriplet> compute_diff_vector(void);

    /////////////////////////////////////////////////////////////////////////////
    /// \brief Returns the states of the DIFF-API as a vector of diff triplets.
    ///        An empty vector means there is no diff.
    /// \details Note: Does not compute a diff itself only creates the triplet 
    ///          from the DIFF-API!
    std::vector<vrtlfi::td::UniqueElementTriplet> gen_nz_triplet_vec(void) const;

    /////////////////////////////////////////////////////////////////////////////
    /// \brief Constructor. Attach existing VrtlmodApis to this Differential
    )" << api_name
      << "Differential(const " << api_name << "& faulty, const " << api_name << R"(& reference);
)"
      << R"(
};

#endif /*__)"
      << top_type << "_VRTLMOD_DIFFAPI_HPP__ */";

    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
