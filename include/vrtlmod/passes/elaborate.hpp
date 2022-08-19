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
/// @file elaborate.hpp
/// @date Created on Tue Mar 15 15:09:12 2022
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_PASSES_ELABORATE_HPP__
#define __VRTLMOD_PASSES_ELABORATE_HPP__

#include "vrtlmod/passes/pass.hpp"
#include "vrtlmod/core/vrtlparse.hpp"

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all core vrtlmod functionalities
namespace vrtlmod
{
////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for passes that act on vrtlmod core data during LLVM/Clang parsing of VRTL
namespace passes
{
////////////////////////////////////////////////////////////////////////////////
/// @class ElaboratePass
/// @brief Elaborates a given VRTL code base. Builds a [cells->cells][modules->{signals,cells}] tree
class ElaboratePass final : public VrtlmodPass
{
  public:
    ///////////////////////////////////////////////////////////////////////
    /// \brief VrtlmodPass action during AST traversal in VrtlParse
    void action(const VrtlParser &parser, const clang::ast_matchers::MatchFinder::MatchResult &Result) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    /// \param core Reference to vrtlmod core storing signal and injection data
    ElaboratePass(const VrtlmodCore &core);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Destructor
    virtual ~ElaboratePass(void) {}
};

} // namespace passes
} // namespace vrtlmod

#endif // __VRTLMOD_PASSES_ELABORATE_HPP__
