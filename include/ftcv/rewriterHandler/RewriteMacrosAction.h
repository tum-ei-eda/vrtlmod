////////////////////////////////////////////////////////////////////////////////
/// @file ReWriteMacrosAction.h
////////////////////////////////////////////////////////////////////////////////

#ifndef FTCV_REWRITEMACROSACTION_H
#define FTCV_REWRITEMACROSACTION_H

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

namespace ftcv {

class RewriteMacrosAction : public clang::PreprocessorFrontendAction
{
	public:
		RewriteMacrosAction();
		void ExecuteAction();
	public:
		static bool cleanFile(const std::string & file);
};

} // namespace ftcv

#endif // FTCV_REWRITEMACROSACTION_H
