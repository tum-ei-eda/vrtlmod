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
/// @file analyze.cpp
/// @date Created on Wed Mar 02 13:09:11 2022
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/passes/analyze.hpp"
#include "vrtlmod/core/types.hpp"

#include "vrtlmod/util/logging.hpp"

namespace vrtlmod
{
namespace passes
{

void AnalyzePass::action(const VrtlParser &parser, const clang::ast_matchers::MatchFinder::MatchResult &Result) const
{
    auto ctx = Result.Context;
    // auto &srcmgr = ctx->getSourceManager();
    // auto &lang_opts = ctx->getLangOpts();

    if (const clang::ArraySubscriptExpr *x = Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("array"))
    {
        if (parser.is_in_sequent(x, ctx))
        {
            LOG_VERBOSE("{array}: ", parser.get_source_code_str(x), "\n",
                        util::logging::dump_to_str<const clang::Stmt *>(x, ctx));
        }
    }
    if (const clang::CXXOperatorCallExpr *x = Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("oop"))
    {
        if (parser.is_in_sequent(x, ctx))
        {
            LOG_VERBOSE("{oop}: ", parser.get_source_code_str(x), "\n",
                        util::logging::dump_to_str<const clang::Stmt *>(x, ctx));
        }
    }
    if (const clang::Expr *x = Result.Nodes.getNodeAs<clang::Expr>("symboltable"))
    {
        LOG_VERBOSE("{symboltable}: ", parser.get_source_code_str(x));
        if (const clang::MemberExpr *instance = Result.Nodes.getNodeAs<clang::MemberExpr>("instance"))
        {
            if (const auto *module = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("module"))
            {
                LOG_VERBOSE(">{instance}: ", parser.get_source_code_str(instance), ", aka ",
                            instance->getMemberDecl()->getNameAsString(), " of {module}: ", module->getNameAsString());
                get_core().add_module_instance(instance, module, *ctx);
            }
        }
    }

    auto check_add_seqassignment = [&](const auto &expr)
    {
        auto assignee_parent_pair = parser.parse_sequential_assignment(expr, Result);

        auto &assignee = assignee_parent_pair.first;
        auto &parent = assignee_parent_pair.second;

        if (assignee != nullptr && parent != nullptr)
        {
            LOG_VERBOSE("check target status of: `{signal} ", parser.get_source_code_str(assignee), " aka `",
                        assignee->getMemberNameInfo().getAsString(), "` of `{module}:", parent->getNameAsString(), "`");
            if (const types::Variable *var = get_core().add_injection_location(assignee, parent, *ctx))
            {
                LOG_INFO("injection location for [", var->get_id(), "] found assignment:\n  \\- ",
                         parser.get_source_code_str(expr), " at: ", var->get_inj_loc());
                LOG_VERBOSE("AST:\n", util::logging::dump_to_str<const clang::Stmt *>(expr, ctx));
            }
        }
    };

    clang::BinaryOperator const * binop = nullptr;

    if (binop = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_array_oop_oop_3d"))
    {
        LOG_INFO("{sea_binary_array_oop_oop_3d}:", util::logging::dump_to_str<const clang::Stmt *>(binop, ctx));
    }
    else if (binop = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_oop_3d"))
    {
        LOG_INFO("{sea_binary_oop_3d}:", util::logging::dump_to_str<const clang::Stmt *>(binop, ctx));
    }
    else if (binop = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_array_3d"))
    {
        LOG_INFO("{sea_binary_array_3d}:", util::logging::dump_to_str<const clang::Stmt *>(binop, ctx));
        #if VRTLMOD_VERILATOR_VERSION <= 4202
        #else // VERILATOR_VERSION <= 4.228
        LOG_FATAL("Matched unexpected sequential assignment!");
        #endif
    }
    else if (binop = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_array_oop_2d"))
    {
        LOG_INFO("{sea_binary_array_oop_2d}:", util::logging::dump_to_str<const clang::Stmt *>(binop, ctx));
    }
    else if (binop = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_oop_2d"))
    {
        LOG_INFO("{sea_binary_oop_2d}:", util::logging::dump_to_str<const clang::Stmt *>(binop, ctx));
    }
    else if (binop = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_array_2d"))
    {
        LOG_INFO("{sea_binary_array_2d}:", util::logging::dump_to_str<const clang::Stmt *>(binop, ctx));
        #if VRTLMOD_VERILATOR_VERSION <= 4202
        #else // VERILATOR_VERSION <= 4.228
        LOG_FATAL("Matched unexpected sequential assignment!");
        #endif
    }
    else if (binop = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_oop_1d"))
    {
        LOG_INFO("{sea_binary_oop_1d}:", util::logging::dump_to_str<const clang::Stmt *>(binop, ctx));
    }
    else if (binop = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_array_1d"))
    {
        LOG_INFO("{sea_binary_array_1d}:", util::logging::dump_to_str<const clang::Stmt *>(binop, ctx));
    }
    else if (binop = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_trivial"))
    {
        LOG_INFO("{sea_binary_trivial}:", util::logging::dump_to_str<const clang::Stmt *>(binop, ctx));
    }

    if(binop != nullptr)
    {
        check_add_seqassignment(binop);
    }

    if (const clang::CallExpr *x = Result.Nodes.getNodeAs<clang::CallExpr>("sea_func"))
    {
        if (const clang::Expr *arg = Result.Nodes.getNodeAs<clang::Expr>("arg1_expr"))
        {
            check_add_seqassignment(x);
        }
    }
}

AnalyzePass::AnalyzePass(const VrtlmodCore &core) : VrtlmodPass(core) {}

} // namespace passes
} // namespace vrtlmod
