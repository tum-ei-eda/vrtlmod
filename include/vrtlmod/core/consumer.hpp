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
/// @file consumer.h
/// @modified on Wed Dec 09 13:32:12 2020
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_TRANSFORM_CONSUMER_HPP__
#define __VRTLMOD_TRANSFORM_CONSUMER_HPP__

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

#include "vrtlmod/util/logging.hpp"
#include "vrtlmod/core/filecontext.hpp"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <initializer_list>

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all core vrtlmod functionalities
namespace vrtlmod
{

class Consumer;

// Handler parent class to find nodes in AST (like statements or expressions).
class Handler : public clang::ast_matchers::MatchFinder::MatchCallback
{
  public:
    Handler(Consumer &consumer); // Constructor sets Consumer
    virtual ~Handler();

    virtual void addMatcher(clang::ast_matchers::MatchFinder &finder) = 0;

    virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) = 0;

    clang::Rewriter &getRewriter() const;           // returns rewrite of consumer
    const clang::ASTContext &getASTContext() const; // returns context of consumer & aborts if context does not exist

    // Set signals of consumer
    void signalChange();
    void signalIncompatibleChange();
    bool hasIncompatibleChange();

    bool inThisFile(clang::SourceLocation sl); // Checks if given sl is in actual file

  private:
    Consumer &consumer_;

  public:
};

class Consumer : public clang::ASTConsumer
{
    friend class Handler;
    template <typename T>
    friend std::string util::logging::toLogString(const T &cons);

  public:
    Consumer(clang::Rewriter &rw, const std::string &file); // sets File context
    virtual ~Consumer(){};

    void ownHandler(
        std::unique_ptr<Handler> handler); // Add handler to consumer and matcher of this consumer to handler

    virtual void Initialize(clang::ASTContext &Context);

    virtual void HandleTranslationUnit(clang::ASTContext &Context);

  private:
    FileContext fc_;
    clang::ast_matchers::MatchFinder matcher_;
    std::list<std::unique_ptr<Handler>> handlers_;
};

} // namespace vrtlmod
namespace util
{
namespace logging
{
template <>
std::string toLogString<vrtlmod::Consumer>(const vrtlmod::Consumer &cons);
} // namespace logging
} // namespace util

#endif // __VRTLMOD_TRANSFORM_CONSUMER_HPP__
