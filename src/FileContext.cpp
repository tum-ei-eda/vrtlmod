////////////////////////////////////////////////////////////////////////////////
/// @file FileContext.cpp
////////////////////////////////////////////////////////////////////////////////

#include "ftcv/FileContext.h"

namespace ftcv {

FileContext::FileContext(clang::Rewriter &rw, const std::string &file) : rewriter_(rw), file_(file), context_(0), changed_(false), incompatibleChange_(
		false) {

}

FileContext::~FileContext() {

}

bool FileContext::anyChange_ = false;

bool FileContext::anyChange() {
	return anyChange_;
}

bool FileContext::fatalFailure_ = false;

void FileContext::resetAnyChangeFlag() {
	anyChange_ = false;
}

bool FileContext::fatalFailure() {
	return fatalFailure_;
}

} // namespace ftcv
