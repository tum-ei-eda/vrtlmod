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
/// @file types.cpp
/// @date Created on Mon Jan 10 18:21:20 2020
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/core/types.hpp"

#include "vrtlmod/util/logging.hpp"
#include "vrtlmod/util/utility.hpp"

#include <sstream>

#include "verilated.h"

namespace vrtlmod
{
namespace types
{

std::string Target::get_hierarchy(void) const
{
    return get_id();
}

std::string Target::get_hierarchyDedotted(void) const
{
    return get_id();
}

std::string Target::_self(void) const
{
    return util::concat(get_class(), " ", types::Module(parent()).get_id(), "::", get_id(), "[",
                        std::to_string(get_bits()), "] ", get_cxx_type());
}

bool Target::is_declared_here(const clang::FieldDecl *decl) const
{
    std::string decl_name = decl->getName().str();
    std::string decl_ancestor_name = decl->getParent()->getName().str();
    LOG_VERBOSE("Target::is_declared_here: ", get_id(), " == ", decl_name, "?");
    if (get_id() == decl_name)
    {
        if (types::Module(parent()).get_id() == decl_ancestor_name)
        {
            LOG_VERBOSE("Target::is_declared_here: ", types::Module(parent()).get_id(), " == ", decl_ancestor_name,
                        "?");
            return true;
        }
    }
    return false;
}

bool Target::is_assigned_here(const clang::MemberExpr *assignee, const clang::CXXRecordDecl *parent) const
{
    std::string decl_name = assignee->getMemberNameInfo().getAsString();
    std::string decl_ancestor_name =
        static_cast<const clang::FieldDecl *>(assignee->getMemberDecl())->getParent()->getName().str();

    if (get_id() == decl_name)
    {
        if (types::Module(pugi::xml_node::parent()).get_id() == decl_ancestor_name)
        {
            return true;
        }
    }

    return false;
}

bool Target::check_declared_here(const clang::FieldDecl *decl)
{
    bool ret = is_declared_here(decl);
    found_decl_ |= ret;

    return ret;
}

bool Target::check_assigned_here(const clang::MemberExpr *assignee, const clang::CXXRecordDecl *parent)
{
    bool ret = is_assigned_here(assignee, parent);
    found_assignment_ |= ret;

    return ret;
}

std::vector<std::string> Target::get_cxx_dimension_types(void) const
{
    return get_cxx_dimensions().first;
}

std::vector<int> Target::get_cxx_dimension_lengths(void) const
{
    return get_cxx_dimensions().second;
}

inline std::pair<int, int> Target::get_element_msb_lsb_pair(void) const
{
    int msb = -1, lsb = -1;
    std::string s, dimstr = get_dimensions();
    auto brOpen = dimstr.find('[');
    auto brClose = dimstr.rfind(']');
    dimstr = dimstr.substr(brOpen + 1, brClose - brOpen - 1);
    dimstr.erase(remove_if(dimstr.begin(), dimstr.end(), isspace), dimstr.end());
    std::istringstream x(dimstr);
    while (getline(x, s, ','))
    {
        s = s.substr(s.find("'") + 1, s.rfind("'") - s.find("'") - 1);
        if (s.find(':') != std::string::npos)
        {
            std::string up = s.substr(0, s.find(':'));
            std::string to = s.substr(s.find(':') + 1);
            msb = boost::lexical_cast<int>(up);
            lsb = boost::lexical_cast<int>(to);
            break;
        }
    }
    return {msb, lsb};
}

inline std::vector<int> Target::get_dimension_lengths(void) const
{
    std::vector<int> dim{};
    std::string s, dimstr = get_dimensions();
    auto brOpen = dimstr.find('[');
    auto brClose = dimstr.rfind(']');
    dimstr = dimstr.substr(brOpen + 1, brClose - brOpen - 1);
    dimstr.erase(remove_if(dimstr.begin(), dimstr.end(), isspace), dimstr.end());
    std::istringstream x(dimstr);
    while (getline(x, s, ','))
    {
        s = s.substr(s.find("'") + 1, s.rfind("'") - s.find("'") - 1);
        if (s.find(':') == std::string::npos)
            dim.push_back(boost::lexical_cast<int>(s));
        else
        {
            std::string up = s.substr(0, s.find(':'));
            std::string to = s.substr(s.find(':') + 1);
            int iup = boost::lexical_cast<int>(up);
            int ito = boost::lexical_cast<int>(to);
            dim.push_back(iup - ito + 1);
        }
    }
    return dim;
}

inline std::pair<std::vector<std::string>, std::vector<int>> Target::get_cxx_dimensions(void) const
{
    std::pair<std::vector<std::string>, std::vector<int>> dim{};
    auto &cxxtypedim = dim.first;
    auto &cxxdim = dim.second;
    cxxdim = get_dimension_lengths();

    std::string s, basetypestr = get_bases();

    auto brOpen = basetypestr.find('[');
    auto brClose = basetypestr.rfind(']');
    basetypestr = basetypestr.substr(brOpen + 1, brClose - brOpen - 1);
    basetypestr.erase(remove_if(basetypestr.begin(), basetypestr.end(), isspace), basetypestr.end());
    std::istringstream basetypestream(basetypestr);
    while (getline(basetypestream, s, ','))
    {
        s = s.substr(s.find("'") + 1, s.rfind("'") - s.find("'") - 1);
        cxxtypedim.push_back(s);
    }

    if (cxxtypedim.back().find("VlWide<") != std::string::npos)
    {
        auto dimstr = cxxtypedim.back();
        brOpen = dimstr.find('<');
        brClose = dimstr.rfind('>');
        dimstr = dimstr.substr(brOpen + 1, brClose - brOpen - 1);
        cxxdim.back() = boost::lexical_cast<int>(dimstr);
        cxxtypedim.push_back("EData");
    }
    else if (cxxtypedim.back().find("WData[") != std::string::npos)
    {
        auto dimstr = cxxtypedim.back();
        brOpen = dimstr.find('[');
        brClose = dimstr.rfind(']');
        dimstr = dimstr.substr(brOpen + 1, brClose - brOpen - 1);
        cxxdim.back() = boost::lexical_cast<int>(dimstr);
        cxxtypedim.push_back("EData");
    }
    else
    { // remove last cxxdim_ element (numberof1dimbits)
        cxxdim.pop_back();
    }

    return dim;
}

unsigned long Target::get_element_mask(std::initializer_list<size_t> subscripts) const
{
    unsigned long mask = 0;
    auto dimlens = get_cxx_dimension_lengths();

    if (subscripts.size() != dimlens.size())
    {
        std::string il = "{ ";
        for (const auto &it : subscripts)
            il += std::to_string(it);
        il += " ";

        il += "}";

        LOG_FATAL("Target get element mask, element initializer list ", il, " does not fit target ", this->_self());
        return -1;
    }

    auto one_dim_bits = get_one_dim_bits();
    auto msblsb_pair = get_element_msb_lsb_pair();
    auto msb = msblsb_pair.first;
    auto lsb = msblsb_pair.second;

    if (one_dim_bits > sizeof(QData)*8) //(cxxtypedim.back().find("WData[") != std::string::npos) //(one_dim_bits > 64)
    {
        int elementsubs = *(subscripts.end()-1);
        int elementsize = dimlens.back();
        // packed in WData
        if ((elementsubs + 1) != elementsize)
        {
            mask = WData(-1);
        }
        else
        {
            for (int i = 0; i < one_dim_bits%(sizeof(WData)*8); ++i)
                mask |= 1 << i;
        }
    }
    else
    {
        // unpacked
        for (int i = lsb; i < msb+1; ++i)
            mask |= 1 << i;
    }
    return mask;
}

} // namespace types
} // namespace vrtlmod
