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
/// @file pass.hpp
/// @date Created on Mon Feb 28 14:31:40 2022
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_PASSES_PASS_HPP__
#define __VRTLMOD_PASSES_PASS_HPP__

#include "vrtlmod/core/core.hpp"

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all core vrtlmod functionalities
namespace vrtlmod
{

class VrtlParser;

class VrtlmodPass
{
    const VrtlmodCore &vrtlmod_core_;

  public:
    ///////////////////////////////////////////////////////////////////////
    /// \brief execute some code at end of translation
    virtual void end_of_translation(const VrtlParser &parser) const {(void) parser;}
    ///////////////////////////////////////////////////////////////////////
    /// \brief Get reference to VrtlmodCore
    const VrtlmodCore &get_core() const { return vrtlmod_core_; }
    ///////////////////////////////////////////////////////////////////////
    /// \brief VrtlmodPass action during AST traversalin VrtlParse. Implement this in the specific!
    virtual void action(const VrtlParser &parser,
                        const clang::ast_matchers::MatchFinder::MatchResult &Result) const = 0;

    VrtlmodPass(const VrtlmodCore &vrtlmod_core) : vrtlmod_core_(vrtlmod_core) {}
    virtual ~VrtlmodPass() {}
};

} // namespace vrtlmod

#endif // __VRTLMOD_CORE_PASS_HPP__
