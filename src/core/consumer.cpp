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
/// @file consumer.cpp
/// @modified on Wed Dec 09 13:32:12 2020
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/core/consumer.hpp"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

namespace vrtlmod
{

Handler::Handler(Consumer &consumer) : consumer_(consumer) {}
Handler::~Handler() {}

Rewriter &Handler::getRewriter() const
{
    return consumer_.fc_.rewriter_;
}

const clang::ASTContext &Handler::getASTContext() const
{
    clang::ASTContext *ret = consumer_.fc_.context_;
    if (ret == 0)
        LOG_FATAL("Failed to retrieve Clang AST context");
    return *ret;
}

void Handler::signalChange()
{
    consumer_.fc_.signalChange();
}
void Handler::signalIncompatibleChange()
{
    consumer_.fc_.signalIncompatibleChange();
}
bool Handler::hasIncompatibleChange()
{
    return consumer_.fc_.hasIncompatibleChange();
}

bool Handler::inThisFile(clang::SourceLocation sl)
{
    return getRewriter().getSourceMgr().getFileID(sl) == getRewriter().getSourceMgr().getMainFileID();
}

Consumer::Consumer(clang::Rewriter &rw, const std::string &file) : fc_(rw, file) {}

void Consumer::ownHandler(std::unique_ptr<Handler> handler)
{
    handler->addMatcher(matcher_);
    handlers_.push_back(std::move(handler));
}

void Consumer::Initialize(ASTContext &Context) {}

void Consumer::HandleTranslationUnit(ASTContext &Context)
{
    fc_.context_ = &Context;
    matcher_.matchAST(Context);
    fc_.context_ = 0;
}

} // namespace vrtlmod

namespace util
{
namespace logging
{
template <>
std::string toLogString<vrtlmod::Consumer>(const vrtlmod::Consumer &cons)
{
    return std::string("{vrtlmod::Consumer file=") + cons.fc_.file_ + "}";
}
} // namespace logging
} // namespace util
