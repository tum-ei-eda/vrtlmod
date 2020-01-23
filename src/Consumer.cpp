////////////////////////////////////////////////////////////////////////////////
/// @file Consumer.cpp
////////////////////////////////////////////////////////////////////////////////

#include "ftcv/Consumer.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

namespace ftcv {

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
		ftcv::abort();
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

template<> std::string toLogString<Consumer>(const Consumer &cons) {
	return std::string("{ftcv::Consumer file=") + cons.fc_.file_ + "}";
}
template<> std::string toLogString<std::tuple<const clang::SourceRange&, clang::SourceManager&> >(
		const std::tuple<const clang::SourceRange&, clang::SourceManager&> &cons) {
	return std::string("{clang::SourceRange {")
			+ toLogString(std::tie<const clang::SourceLocation&, clang::SourceManager&>(std::get<0>(cons).getBegin(), std::get<1>(cons)))
			+ ","
			+ toLogString(std::tie<const clang::SourceLocation&, clang::SourceManager&>(std::get<0>(cons).getEnd(), std::get<1>(cons)))
			+ "}";
}
template<> std::string toLogString<std::tuple<const clang::SourceLocation&, clang::SourceManager&> >(
		const std::tuple<const clang::SourceLocation&, clang::SourceManager&> &cons) {
	std::stringstream ss;
	{
		llvm::raw_os_ostream rout(ss);
		std::get<0>(cons).print(rout, std::get<1>(cons));
		rout.flush();
	}
	return ss.str();
}

} // namespace ftcv
