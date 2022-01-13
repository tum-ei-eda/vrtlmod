/*
 * Copyright 2021 Chair of EDA, Technical University of Munich
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
