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
/// @file signaldeclrewrite.hpp
/// @date Created on Wed Feb 02 17:46:20 2022
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_PASSES_SIGNALDECLREWRITE_HPP__
#define __VRTLMOD_PASSES_SIGNALDECLREWRITE_HPP__

#include "vrtlmod/passes/pass.hpp"
#include "vrtlmod/core/vrtlparse.hpp"

#include <vector>

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all core vrtlmod functionalities
namespace vrtlmod
{
////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for passes that act on vrtlmod core data during LLVM/Clang parsing of VRTL
namespace passes
{
////////////////////////////////////////////////////////////////////////////////
/// @class SignalDeclRewriter
/// @brief AST Handler/Consumer. Rewrites VRTL source code signal declaration by extending verilated modules with
/// reference bases injection points
////////////////////////////////////////////////////////////////////////////////
class SignalDeclRewriter : public VrtlmodPass
{
  protected:
    void write_injection_decl(const clang::FieldDecl *dec, const types::Target &t, const VrtlParser &parser) const;
    void modify_includes(const clang::Decl *decl, const VrtlParser &parser) const;

  public:
    ///////////////////////////////////////////////////////////////////////
    /// \brief VrtlmodPass action during AST traversal in VrtlParse
    void action(const VrtlParser &parser, const clang::ast_matchers::MatchFinder::MatchResult &Result) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Prints analysis of rewrite work done
    void analyzeRewrite(void);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    /// \param core Reference to vrtlmod core storing signal and injection data
    SignalDeclRewriter(const VrtlmodCore &core);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Destructor
    virtual ~SignalDeclRewriter(void);

  private:
    std::set<clang::SourceLocation> visited;
};

} // namespace passes
} // namespace vrtlmod

#endif // __VRTLMOD_TRANSFORM_SIGNALDECLREWRITE_HPP__
