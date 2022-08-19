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
/// @file injectionrewrite.hpp
/// @date Created on Mon Jan 09 11:56:10 2020
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_PASSES_INJECTIONREWRITE_HPP__
#define __VRTLMOD_PASSES_INJECTIONREWRITE_HPP__

#include "vrtlmod/passes/pass.hpp"
#include "vrtlmod/core/vrtlparse.hpp"
#include "vrtlmod/core/types.hpp"

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all core vrtlmod functionalities
namespace vrtlmod
{
////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for passes that act on vrtlmod core data during LLVM/Clang parsing of VRTL
namespace passes
{
////////////////////////////////////////////////////////////////////////////////
/// @class InjectionRewriter
/// @brief Elaborates a given VRTL code base. Builds a [cells->cells][modules->{signals,cells}] tree
class InjectionRewriter final : public VrtlmodPass
{
    struct SInj
    {
        virtual void rewrite_injection(const VrtlParser &parser) const = 0;
        std::string prefix_;
        const types::Target &t_;
        SInj(const std::string &prefix, const types::Target &t) : prefix_(prefix), t_(t) {}
        virtual ~SInj(void) {}
    };
    struct BinarySInj : SInj
    {
        void rewrite_injection(const VrtlParser &parser) const override;
        const clang::BinaryOperator *expr_;
        BinarySInj(const clang::BinaryOperator *expr, const std::string &prefix, const types::Target &t)
            : SInj(prefix, t), expr_(expr)
        {
        }
        virtual ~BinarySInj(void) {}
    };
    struct CallSInj : SInj
    {
        void rewrite_injection(const VrtlParser &parser) const override;
        const clang::CallExpr *expr_;
        const clang::Expr *arg_;
        CallSInj(const clang::CallExpr *expr, const clang::Expr *arg, const std::string &prefix, const types::Target &t)
            : SInj(prefix, t), expr_(expr), arg_(arg)
        {
        }
        virtual ~CallSInj(void) {}
    };

    mutable std::map<const clang::FunctionDecl *, std::vector<std::shared_ptr<SInj>>> map_sequential_injections_;

    mutable const clang::FunctionDecl *active_sequent_func_{ nullptr }; ///< active sequential function

    mutable std::map<const clang::FunctionDecl *, std::set<std::pair<std::string, const types::Target *>>>
        map_injected_targets_; ///< map keyed with sequential functions valued with pairs of targets and their
                               ///< function-local prefix, prefix is required because some Verilated functions are
                               ///< static, thus, not allowing `this->`
    void modify_includes(const clang::Expr *expr, const VrtlParser &parser) const {}

  public:
    void end_of_translation(const VrtlParser &parser) const override;
    void wrap_up_sequent_function(const clang::FunctionDecl *func, const VrtlParser &parser) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief VrtlmodPass action during AST traversal in VrtlParse
    void action(const VrtlParser &parser, const clang::ast_matchers::MatchFinder::MatchResult &Result) const;
    ///////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    /// \param core Reference to vrtlmod core storing signal and injection data
    InjectionRewriter(const VrtlmodCore &core);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Destructor
    virtual ~InjectionRewriter(void);
};

} // namespace passes
} // namespace vrtlmod

#endif // __VRTLMOD_PASSES_INJECTIONREWRITE_HPP__
