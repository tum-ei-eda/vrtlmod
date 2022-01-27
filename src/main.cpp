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
/// @file main.cpp
/// @brief main file for llvm-based VRTL-modifer tool
/// @details based on ftcv frontend by ?
/// @date Created on Mon Jan 15 12:29:21 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;

#include "vrtlmod/vrtlmod.hpp"
#include "vrtlmod/util/system.hpp"

////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option category
static llvm::cl::OptionCategory UserCat("User Options");
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "systemc".
static llvm::cl::opt<bool> SystemC("systemc", llvm::cl::Optional, llvm::cl::desc("Input VRTL is SystemC code"),
                                   llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "regxml". Sets input Xml
static llvm::cl::opt<std::string> RegisterXmlFilename("regxml", llvm::cl::Required,
                                                      llvm::cl::desc("Specify input register xml"),
                                                      llvm::cl::value_desc("file name"), llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "out". Sets output directory path
static llvm::cl::opt<std::string> OUTdir("out", llvm::cl::Required, llvm::cl::desc("Specify output directory"),
                                         llvm::cl::value_desc("path"), llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "override".
static llvm::cl::opt<bool> Overwrite("overwrite", llvm::cl::Optional, llvm::cl::desc("Override source files"),
                                     llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "silent".
static llvm::cl::opt<bool> Silent("silent", llvm::cl::Optional, llvm::cl::desc("Execute without Warnings and Info"),
                                  llvm::cl::cat(UserCat));

static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
// static llvm::cl::extrahelp MoreHelp(vrtlmod::env::get_environmenthelp().c_str());

////////////////////////////////////////////////////////////////////////////////
/// \brief vrtlmod main()
int main(int argc, const char **argv)
{
    // Consume arguments

    CommonOptionsParser op(argc, argv, UserCat);

    if (bool(Silent))
    {
        util::logging::log(util::logging::OBLIGAT, "Executing silently - Warnings and Infos disabled", true);
    }

    if (!fs::exists(OUTdir.c_str()))
    {
        util::logging::log(util::logging::WARNING,
                           std::string("Output directory ") + std::string(OUTdir) + " doesn't exist!");
    }

    if (!fs::exists(RegisterXmlFilename.c_str()))
    {
        util::logging::abort(std::string("XML file ") + std::string(RegisterXmlFilename) + " doesn't exist!");
    }

    vapi::VapiGenerator &tAPI = vapi::VapiGenerator::_i();
    if (tAPI.init(RegisterXmlFilename.c_str(), OUTdir.c_str(), SystemC) < 0)
    {
        util::logging::abort(std::string("Vrtlmod API generator initialization failed"));
    }

    std::vector<std::string> sources = op.getSourcePathList();

    // prepare *_vrtlmod.cpp files: create, de-macro, clean comments.
    sources = tAPI.prepare_sources(sources, Overwrite);

    // create a new Clang Tool instance
    ClangTool ToolM(op.getCompilations(), sources);

    // run Clang // insert macros (needed for macro code rewrite)
    int err = ToolM.run(newFrontendActionFactory<transform::rewrite::RewriteMacrosAction>().get());

    for (size_t i = 0; i < sources.size(); ++i)
    {
        transform::rewrite::RewriteMacrosAction::cleanFile(sources[i]);
    }

    ClangTool ToolRw(op.getCompilations(), sources);

    err = ToolRw.run(newFrontendActionFactory<MyFrontendAction>().get());

    tAPI.build_API();

    return (0);
}
