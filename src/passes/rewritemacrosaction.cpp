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
/// @file rewritemacrosaction.cpp
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/passes/rewritemacrosaction.hpp"

#include "vrtlmod/util/logging.hpp"
#include "vrtlmod/util/utility.hpp"

namespace vrtlmod
{
namespace transform
{
namespace rewrite
{

RewriteMacrosAction::RewriteMacrosAction() {}

void RewriteMacrosAction::ExecuteAction()
{
    LOG_VERBOSE("> Rewrite Macros file", getCurrentFile().str());

    clang::CompilerInstance &CI = getCompilerInstance();
    std::stringstream buffer;
    llvm::raw_os_ostream rout(buffer);
    RewriteMacrosInInput(CI.getPreprocessor(), &rout);
    rout.flush();
    std::ofstream out(getCurrentFile().str().c_str());
    out << buffer.str();
    out.flush();
    out.close();
}

} // namespace rewrite
} // namespace transform
} // namespace vrtlmod
