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
/// @file Consumer.cpp
/// @date Created on ?
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author ?
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/transform/consumer.hpp"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

namespace transform {

Handler::Handler(Consumer &consumer) : consumer_(consumer) {

}
Handler::~Handler() {

}

Rewriter& Handler::getRewriter() {
	return consumer_.fc_.rewriter_;
}
clang::ASTContext& Handler::getASTContext() {
	clang::ASTContext *ret = consumer_.fc_.context_;
	if (ret == 0)
		util::logging::abort();
	return *ret;
}

void Handler::signalChange() {
	consumer_.fc_.signalChange();
}
void Handler::signalIncompatibleChange() {
	consumer_.fc_.signalIncompatibleChange();
}
bool Handler::hasIncompatibleChange() {
	return consumer_.fc_.hasIncompatibleChange();
}

bool Handler::inThisFile(clang::SourceLocation sl) {
	return getRewriter().getSourceMgr().getFileID(sl) == getRewriter().getSourceMgr().getMainFileID();
}

Consumer::Consumer(clang::Rewriter &rw, const std::string &file) : fc_(rw, file) {

}
Consumer::~Consumer() {
	for (std::list<Handler*>::iterator c = handlers_.begin(); c != handlers_.end(); c++) {
		delete *c;
	}

}

void Consumer::ownHandler(Handler *handler) {
	if (handler == 0)
		return;
	handler->addMatcher(matcher_);
	handlers_.push_back(handler);
}

void Consumer::Initialize(ASTContext &Context) {

	//llvm::OwningPtr<clang::ExternalASTSource> aaor(new ftcv::ArrayAccessOperatorRewriter(fc_));

	//Context.setExternalSource(aaor);
}

void Consumer::HandleTranslationUnit(ASTContext &Context) {
	fc_.context_ = &Context;
	matcher_.matchAST(Context);
	fc_.context_ = 0;
}

} // namespace transform
