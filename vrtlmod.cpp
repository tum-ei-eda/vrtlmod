////////////////////////////////////////////////////////////////////////////////
/// @file vrtlmod.cpp
/// @brief main file for llvm-based VRTL-modifer tool
/// @details based on ftcv frontend
/// @date Created on Mon Jan 15 12:29:21 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

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

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

#include "APIbuild/apibuilder.hpp"
#include "APIbuild/injectionrewriter.hpp"

static llvm::cl::OptionCategory MatcherSampleCategory("Matcher Sample");

// For each source file provided to the tool, a new FrontendAction is created.
// It allows to execute actions on the AST tree
// Used as long as changes occur.
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

llvm::cl::OptionCategory UserCat("User Options");
llvm::cl::opt<std::string> RegisterXmlFilename("regxml", llvm::cl::Required, llvm::cl::desc("Specify input register xml"),
		llvm::cl::value_desc("file name"), llvm::cl::cat(UserCat));
llvm::cl::opt<std::string> OUTdir("out", llvm::cl::Optional, llvm::cl::desc("Specify output directory"), llvm::cl::value_desc("path"),
		llvm::cl::cat(UserCat));

int main(int argc, const char **argv) {
	// Consume arguments
	APIbuilder &tAPI = APIbuilder::_i();

	CommonOptionsParser op(argc, argv, UserCat);

	if (tAPI.init(RegisterXmlFilename.c_str(), OUTdir.c_str()) < 0)
		return (-1);

	std::vector<std::string> sources = op.getSourcePathList();

	// prepare *_vrtlmod.cpp files: create, de-macro, clean comments.
	for (size_t i = 0; i < sources.size(); i++) {
		std::stringstream tmp;
		std::string srcName;
		auto lSl = sources[i].rfind("/");
		auto dcpp = sources[i].rfind(".cpp");
		if (lSl != std::string::npos) {
			srcName = sources[i].substr(lSl + 1, dcpp - lSl - 1);
		} else {
			srcName = sources[i].substr(0, dcpp - 1);
		}
		tmp << tAPI.get_outputDir();
		tmp << "/";
		tmp << srcName << "_vrtlmod.cpp";
		system((std::string("cp \"") + sources[i] + "\" \"" + tmp.str() + "\"").c_str());
		sources[i] = tmp.str();
	}

	// create a new Clang Tool instance
	ClangTool ToolM(op.getCompilations(), sources);

	// run Clang // insert macros (needed for macro code rewrite)
	int err = ToolM.run(newFrontendActionFactory<ftcv::RewriteMacrosAction>().get()
			);

	for (size_t i = 0; i < sources.size(); i++) {
		ftcv::RewriteMacrosAction::cleanFile(sources[i]);
	}

	ClangTool ToolRw(op.getCompilations(), sources);

	err = ToolRw.run(newFrontendActionFactory<MyFrontendAction>().get());

	return (err);

}

