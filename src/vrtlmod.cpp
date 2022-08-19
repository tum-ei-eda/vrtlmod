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
/// @file vrtlmod.cpp
/// @date Created on Mon Jan 15 12:29:21 2020
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/vrtlmod.hpp"

#include "clang/AST/AST.h"
#include "clang/AST/ASTDumper.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Frontend/FrontendActions.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/Lexer.h"

#include "vrtlmod/core/consumer.hpp"
#include "vrtlmod/passes/rewritemacrosaction.hpp"
#include "vrtlmod/passes/injectionrewrite.hpp"
#include "vrtlmod/passes/elaborate.hpp"
#include "vrtlmod/passes/analyze.hpp"
#include "vrtlmod/passes/signaldeclrewrite.hpp"
#include "vrtlmod/vapi/generator.hpp"

#include "vrtlmod/core/core.hpp"

namespace vrtlmod
{

const std::string &get_version(void)
{
    static const std::string v = VRTLMOD_VERSION;
    return v;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief custom creator to pass a context instance to the ASTFrontendActions
template <typename T, typename Ctx_t>
std::unique_ptr<clang::tooling::FrontendActionFactory> newGeneratorFrontendActionFactory(Ctx_t &ctx)
{
    class GeneratorFrontendActionFactory : public clang::tooling::FrontendActionFactory
    {
        Ctx_t &ctx_;

      public:
        GeneratorFrontendActionFactory(Ctx_t &ctx) : ctx_(ctx) {}
        std::unique_ptr<clang::FrontendAction> create() override { return std::make_unique<T>(ctx_); }
    };

    return std::make_unique<GeneratorFrontendActionFactory>(ctx);
}

std::unique_ptr<clang::tooling::ToolAction> CreateMacroRewritePass(VrtlmodCore &core)
{
    return clang::tooling::newFrontendActionFactory<vrtlmod::transform::rewrite::RewriteMacrosAction>();
}

std::unique_ptr<clang::tooling::ToolAction> CreateElaboratePass(VrtlmodCore &core)
{
    return newGeneratorFrontendActionFactory<vrtlmod::ParserAction<vrtlmod::passes::ElaboratePass>>(core);
}

std::unique_ptr<clang::tooling::ToolAction> CreateAnalyzePass(VrtlmodCore &core)
{
    return newGeneratorFrontendActionFactory<vrtlmod::ParserAction<vrtlmod::passes::AnalyzePass>>(core);
}

std::unique_ptr<clang::tooling::ToolAction> CreateSignalDeclPass(VrtlmodCore &core)
{
    return newGeneratorFrontendActionFactory<vrtlmod::ParserAction<vrtlmod::passes::SignalDeclRewriter>>(core);
}

std::unique_ptr<clang::tooling::ToolAction> CreateInjectionPass(VrtlmodCore &core)
{
    return newGeneratorFrontendActionFactory<vrtlmod::ParserAction<vrtlmod::passes::InjectionRewriter>>(core);
}

} // namespace vrtlmod
