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
/// @file rewritemacrosaction.hpp
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_PASSES_REWRITEMACROSACTION_HPP__
#define __VRTLMOD_PASSES_REWRITEMACROSACTION_HPP__

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include "clang/AST/AST.h"
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
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all core vrtlmod functionalities
namespace vrtlmod
{
////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for LLVM/Clang source to source transformation
namespace transform
{
////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for LLVM/Clang source to source transformation rewriter tooling
namespace rewrite
{
class RewriteMacrosAction : public clang::PreprocessorFrontendAction
{
  public:
    RewriteMacrosAction();
    void ExecuteAction();
};

class RewriteCommentsAction : public clang::PreprocessorFrontendAction
{
    struct CommentHandler : clang::CommentHandler
    {
        llvm::StringRef file_;
        clang::RewriteBuffer *rb_;
        void set_file(llvm::StringRef file) { file_ = file; }
        void set_rewrite_buffer(clang::RewriteBuffer &RB) { rb_ = &RB; }
        bool HandleComment(clang::Preprocessor &PP, clang::SourceRange Comment) override;
    } ch_;
    void ExecuteAction(void) override;
};

} // namespace rewrite
} // namespace transform
} // namespace vrtlmod

#endif // __VRTLMOD_PASSES_REWRITEMACROSACTION_HPP__
