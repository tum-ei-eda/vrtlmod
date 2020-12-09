////////////////////////////////////////////////////////////////////////////////
/// @file rewritemacrosaction.hpp
/// @date Created on ?
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author ?
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_TRANSFORM_REWRITE_REWRITEMACROSACTION_HPP__
#define __VRTLMOD_TRANSFORM_REWRITE_REWRITEMACROSACTION_HPP__

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
/// @brief namespace for LLVM/Clang source to source transformation
namespace transform {

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for LLVM/Clang source to source transformation rewriter tooling
namespace rewrite {
class RewriteMacrosAction : public clang::PreprocessorFrontendAction
{
	public:
		RewriteMacrosAction();
		void ExecuteAction();
	public:
		static bool cleanFile(const std::string & file);
};

} // namespace transform::rewrite

} // namespace transform

#endif // __VRTLMOD_TRANSFORM_REWRITE_REWRITEMACROSACTION_HPP__
