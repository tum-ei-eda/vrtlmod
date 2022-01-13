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
/// @file filecontext.hpp
/// @date Created on ?
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author ?
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_TRANSFORM_FILECONTEXT_HPP__
#define __VRTLMOD_TRANSFORM_FILECONTEXT_HPP__

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include "clang/AST/AST.h"
#include "clang/Rewrite/Core/Rewriter.h"

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for LLVM/Clang source to source transformation
namespace transform
{

/** \brief Handles status of Files and supports clang rewriter.
 */
class FileContext
{
  public:
    FileContext(clang::Rewriter &rw, const std::string &file);
    ~FileContext();

    inline void signalChange()
    {
        changed_ = true;
        anyChange_ = true;
    }
    inline bool hasChange() { return changed_; }

    inline void resetIncompatibleChangeFlag() { incompatibleChange_ = false; }
    inline bool hasIncompatibleChange() { return incompatibleChange_; }
    inline void signalIncompatibleChange()
    {
        incompatibleChange_ = true;
        anyChange_ = true;
    }
    inline void signalFatalFailure() { fatalFailure_ = true; }

  public:
    clang::Rewriter &rewriter_;
    const std::string file_;
    clang::ASTContext *context_;

  private:
    bool changed_;
    bool incompatibleChange_;
    static bool fatalFailure_;
    static bool anyChange_;

  public:
    static void resetAnyChangeFlag();
    static bool anyChange();
    static bool fatalFailure();
};

} // namespace transform

#endif // __VRTLMOD_TRANSFORM_FILECONTEXT_HPP__
