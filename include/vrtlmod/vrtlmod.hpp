////////////////////////////////////////////////////////////////////////////////
/// @file vrtlmod.hpp
/// @brief main file for llvm-based VRTL-modifer tool
/// @details based on ftcv frontend
/// @date Created on Mon Jan 23 14:33:10 2020
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_VRTLMOD_HPP__
#define __VRTLMOD_VRTLMOD_HPP__

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

#include "vrtlmod/transform/consumer.hpp"
#include "vrtlmod/transform/rewrite/rewritemacrosaction.hpp"
#include "vrtlmod/transform/injectionrewriter.hpp"
#include "vrtlmod/vapi/generator.hpp"

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
		transform::Consumer *cons = new transform::Consumer(rewriter_, curfile);

		cons->ownHandler(new transform::InjectionRewriter(*cons));

		return std::unique_ptr<ASTConsumer>(cons);
	}

private:
	Rewriter rewriter_;
	std::string curfile;
};

namespace vrtlmod {
///////////////////////////////////////////////////////////////////////
/// \brief Prepare sources for transformation
/// \details Copies (or overwrites - corresponding to cmd line options) sources
void prepare_sources(std::vector<std::string>& sources);


namespace env {
///////////////////////////////////////////////////////////////////////
/// \brief Check (Shell) environment for correct specifications
/// \return True if ok, false if not.
bool check_environment(void);
///////////////////////////////////////////////////////////////////////
/// \brief Prints help for setting up the environment
std::string get_environmenthelp(void);
}
}

#endif /* __VRTLMOD_VRTLMOD_HPP__ */
