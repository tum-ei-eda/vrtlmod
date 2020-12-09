////////////////////////////////////////////////////////////////////////////////
/// @file logging.hpp
/// @brief simple misc helper
/// @author ?
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_UTIL_LOGGING_HPP__
#define __VRTLMOD_UTIL_LOGGING_HPP__

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

#include <string>

namespace transform{
	class Consumer;
}

namespace util
{

namespace logging
{

enum LEVEL {
	OBLIGAT,
	INFO,
	WARNING,
	ERROR
 };

const char * toString(LEVEL level);

void log(LEVEL level,const std::string & msg, bool silent_toggle = false);

template <typename T>
std::string toLogString(const T & obj){
	return "{UNKNOWN OBJECT}";
}

template <> inline
std::string toLogString<std::string>(const std::string & obj){
	return std::string("{\"") + obj + "\"}";
}

template<>
std::string toLogString<transform::Consumer>(const transform::Consumer & cons);

template<>
std::string toLogString<std::tuple<const clang::SourceLocation &, clang::SourceManager &> >(const std::tuple<const clang::SourceLocation &, clang::SourceManager& > & cons);

template<>
std::string toLogString<std::tuple<const clang::SourceRange &, clang::SourceManager &> >(const std::tuple<const clang::SourceRange &,clang::SourceManager& > & cons);

template<> inline
std::string toLogString<std::tuple<clang::SourceManager &,const clang::SourceRange &> >(const std::tuple< clang::SourceManager &,const clang::SourceRange &> & cons);


template <typename T,typename ... OT>
void log(LEVEL level,const std::string & msg,const T & o,const OT & ...   objects)
{
	log(level,msg+"\n\t"+toLogString(o),objects...);
}

void abort();
void abort(const std::string & msg);
template <typename T,typename ... OT>
void abort(const std::string & msg,const T & o,const OT & ...   objects)
{
	abort(msg+"\n\t"+toLogString(o),objects...);
}


} // namespace logging

} // namespace util

#endif // __VRTLMOD_UTIL_LOGGING_HPP__
