////////////////////////////////////////////////////////////////////////////////
/// @file Consumer.h
////////////////////////////////////////////////////////////////////////////////

#ifndef FTCV_CONSUMER_H
#define FTCV_CONSUMER_H

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <initializer_list>



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

#include "ftcv/Misc.h"
#include "ftcv/FileContext.h"


namespace ftcv
{

class Consumer;


// Handler parent class to find nodes in AST (like statements or expressions).
class Handler : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    Handler(Consumer & consumer); // Constructor sets Consumer
    virtual ~Handler();

    virtual void addMatcher(clang::ast_matchers::MatchFinder & finder) = 0;

    virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) = 0;

    clang::Rewriter & getRewriter();     // returns rewrite of consumer
    clang::ASTContext & getASTContext(); // returns context of consumer & aborts if context does not exist

    // Set signals of consumer
    void signalChange();
    void signalIncompatibleChange();
    bool hasIncompatibleChange();

    bool inThisFile(clang::SourceLocation sl); // Checks if given sl is in actual file

private:
    Consumer & consumer_;
public:

};

class Consumer : public  clang::ASTConsumer
{
    friend class Handler;
    template<typename T> friend std::string toLogString(const T & cons);
public:

    Consumer(clang::Rewriter & rw,const std::string & file); // sets File context
    ~Consumer(); // Delete handlers

    void ownHandler(Handler * handler); // Add handler to consumer and matcher of this consumer to handler

	virtual void Initialize(clang::ASTContext &Context);

    virtual void HandleTranslationUnit(clang::ASTContext &Context);

private:
    ftcv::FileContext fc_;
    clang::ast_matchers::MatchFinder matcher_;
    std::list<Handler*> handlers_;

};

template<> std::string toLogString<Consumer>(const Consumer & cons);
template<> std::string toLogString<std::tuple<const clang::SourceRange &, clang::SourceManager &> >(const std::tuple<const clang::SourceRange &,clang::SourceManager& > & cons);
template<> inline std::string toLogString<std::tuple<clang::SourceManager &,const clang::SourceRange &> >(const std::tuple< clang::SourceManager &,const clang::SourceRange &> & cons){
	return toLogString<std::tuple<const clang::SourceRange &, clang::SourceManager &> >(std::tie(std::get<1>(cons),std::get<0>(cons)));
}
template<> std::string toLogString<std::tuple<const clang::SourceLocation &, clang::SourceManager &> >(const std::tuple<const clang::SourceLocation &, clang::SourceManager& > & cons);


} // namespace ftcv

#endif // FTCV_CONSUMER_H
