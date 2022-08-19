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
/// @file vrtlmod.hpp
/// @date Created on Mon Jan 23 14:33:10 2020
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_VRTLMOD_HPP__
#define __VRTLMOD_VRTLMOD_HPP__

#include <memory>
#include <string>

namespace clang
{
namespace tooling
{
class ToolAction;
}
class FrontendActionFactory;
} // namespace clang

namespace vrtlmod
{
class VrtlmodCore;
///////////////////////////////////////////////////////////////////////
/// \brief Returns vrtlmod version as string
const std::string &get_version(void);

std::unique_ptr<clang::tooling::ToolAction> CreateMacroRewritePass(VrtlmodCore &core);
std::unique_ptr<clang::tooling::ToolAction> CreateElaboratePass(VrtlmodCore &core);
std::unique_ptr<clang::tooling::ToolAction> CreateAnalyzePass(VrtlmodCore &core);
std::unique_ptr<clang::tooling::ToolAction> CreateSignalDeclPass(VrtlmodCore &core);
std::unique_ptr<clang::tooling::ToolAction> CreateInjectionPass(VrtlmodCore &core);

} // namespace vrtlmod

#endif /* __VRTLMOD_VRTLMOD_HPP__ */
