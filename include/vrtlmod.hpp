////////////////////////////////////////////////////////////////////////////////
/// @file vrtlmod.hpp
/// @brief main file for llvm-based VRTL-modifer tool
/// @details based on ftcv frontend
/// @date Created on Mon Jan 23 14:33:10 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_VRTLMOD_HPP_
#define INCLUDE_VRTLMOD_HPP_

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
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "ftcv/Consumer.h"
#include "ftcv/rewriterHandler/RewriteMacrosAction.h"
#include "APIbuild/apibuilder.hpp"
#include "APIbuild/injectionrewriter.hpp"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

////////////////////////////////////////////////////////////////////////////////
/// @class MyFrontendAction
/// @brief Frontend action for LLVM tool executing actions on the AST tree
/// @author unknown. modified (Johannes Geier)
class MyFrontendAction: public ASTFrontendAction {
public:
	MyFrontendAction() {
	}
	void EndSourceFileAction() {
		rewriter_.overwriteChangedFiles();
	}

	//Add own Consumer and code rewriter to it
	std::unique_ptr<ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef InFile) {
		curfile = InFile.str();
		rewriter_.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
		ftcv::Consumer *cons = new ftcv::Consumer(rewriter_, curfile);

		cons->ownHandler(new apibuild::InjectionRewriter(*cons));

		return std::unique_ptr<ASTConsumer>(cons);
	}

private:
	Rewriter rewriter_;
	std::string curfile;
};



#endif /* INCLUDE_VRTLMOD_HPP_ */
