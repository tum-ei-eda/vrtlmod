////////////////////////////////////////////////////////////////////////////////
/// @file filecontext.cpp
/// @date Created on ?
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author ?
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/transform/filecontext.hpp"

namespace transform {

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

} // namespace transform
