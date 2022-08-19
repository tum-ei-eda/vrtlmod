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
/// @file elaborate.cpp
/// @date Created on Wed Tue 15 15:09:12 2022
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/passes/elaborate.hpp"
#include "vrtlmod/core/types.hpp"

#include "vrtlmod/util/logging.hpp"

namespace vrtlmod
{
namespace passes
{

void ElaboratePass::action(const VrtlParser &parser, const clang::ast_matchers::MatchFinder::MatchResult &Result) const
{
    auto ctx = Result.Context;
    auto &srcmgr = ctx->getSourceManager();
    // auto &lang_opts = ctx->getLangOpts();

    if (const clang::CXXRecordDecl *x = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("module"))
    {
        LOG_VERBOSE("{module}: ", x->getNameAsString());
        if (const auto *module = get_core().add_module(x, *ctx))
        {
            LOG_INFO("{module:} found new module declaration [", module->get_id(), "]");
        }
    }
    if (const clang::FieldDecl *x = Result.Nodes.getNodeAs<clang::FieldDecl>("instance_decl"))
    {
        LOG_VERBOSE("{instance_decl}: ", x->getNameAsString(), "\n  \\-", parser.get_source_code_str(x));
    }
    if (const clang::FieldDecl *x = Result.Nodes.getNodeAs<clang::FieldDecl>("cell_decl"))
    {
        LOG_VERBOSE("{cell_decl}: ", x->getNameAsString(), "\n  \\-", parser.get_source_code_str(x));
        if (const auto *cell = get_core().add_cell(x, *ctx))
        {
            LOG_INFO("{cell}: found cell member [", cell->get_id(), "] of type [", cell->get_type(), "] in module [",
                     types::Module(cell->parent()).get_id(), "]");
        }
    }
    if (const clang::FieldDecl *x = Result.Nodes.getNodeAs<clang::FieldDecl>("topref_decl"))
    {
        LOG_VERBOSE("{topref_decl}: ", x->getNameAsString(), "\n  \\-", parser.get_source_code_str(x));
        if (const auto *module = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("module"))
        {
            if (const auto *cell = get_core().set_top_cell(x, *ctx))
            {
                LOG_INFO("{top cell} found top cell [", cell->get_id(), "] of type [", cell->get_type(), "]");
                get_core().add_module_instance(x, module, *ctx);
            }
        }
    }
    if (const clang::FieldDecl *x = Result.Nodes.getNodeAs<clang::FieldDecl>("signal_decl"))
    {
        LOG_VERBOSE("{signal_decl}: ", x->getNameAsString(), "\n  `\\-", parser.get_source_code_str(x));
        LOG_VERBOSE("AST:\n", util::logging::dump_to_str(x));
        if (const auto *var = get_core().add_variable(x, *ctx, parser.getRewriter()))
        {
            LOG_INFO("{variable}: found variable member [", var->get_id(), "] of type [", var->get_type(),
                     "] in module [", types::Module(var->parent()).get_id(), "]");
        }
    }
}

ElaboratePass::ElaboratePass(const VrtlmodCore &core) : VrtlmodPass(core) {}

} // namespace passes
} // namespace vrtlmod
