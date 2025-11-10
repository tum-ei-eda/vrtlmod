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
/// @date Created on Mon Jan 15 12:29:21 2020
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include "llvm/Support/CommandLine.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "vrtlmod/vrtlmod.hpp"
#include "vrtlmod/core/core.hpp"

#include "vrtlmod/util/utility.hpp"
#include "vrtlmod/util/logging.hpp"

////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option category
static llvm::cl::OptionCategory UserCat("User Options");
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "systemc".
static llvm::cl::opt<bool> SystemC("systemc", llvm::cl::Optional, llvm::cl::desc("Input VRTL is SystemC code"),
                                   llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "wl-regxml". Sets input whitelist Xml
static llvm::cl::opt<std::string> WhiteListXmlFilename("wl-regxml", llvm::cl::Optional,
                                                       llvm::cl::desc("Specify input whitelist register xml"),
                                                       llvm::cl::value_desc("file name"), llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "out". Sets output directory path
static llvm::cl::opt<std::string> OutputDir("out", llvm::cl::Optional, llvm::cl::desc("Specify output directory"),
                                            llvm::cl::value_desc("path"), llvm::cl::init("vrtlmod-out"),
                                            llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "overwrite".
static llvm::cl::opt<bool> Overwrite("overwrite", llvm::cl::Optional, llvm::cl::desc("Override source files"),
                                     llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "silent".
static llvm::cl::opt<bool> Silent("silent", llvm::cl::Optional, llvm::cl::desc("Execute without Warnings and Info"),
                                  llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "xml-only".
static llvm::cl::opt<bool> XmlOnly("xml-only", llvm::cl::Optional,
                                   llvm::cl::desc("Only Elaborate and Analyze the given VRTL"), llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "print-td".
static llvm::cl::opt<bool> PrintTD("print-td", llvm::cl::Optional,
                                   llvm::cl::desc("Print the common targetdictionary header file"),
                                   llvm::cl::cat(UserCat));
static llvm::cl::alias SilentA("s", llvm::cl::NotHidden, llvm::cl::desc("Alias for --silent"),
                               llvm::cl::aliasopt(Silent));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "no-auto-include".
static llvm::cl::opt<bool> NoAutoInclude(
    "no-auto-include", llvm::cl::Optional,
    llvm::cl::desc("Execute without automatic include paths for Verilator and Clang"), llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "diff-unroll".
llvm::cl::opt<bool> DiffApiHardUnroll(
    "diff-unroll", llvm::cl::Optional,
    llvm::cl::desc("When generating the Diff-API code, unroll all multi-dimensional accesses."), llvm::cl::cat(UserCat));
////////////////////////////////////////////////////////////////////////////////
/// \brief Frontend user option "verbose".
static llvm::cl::opt<bool> Verbose("verbose", llvm::cl::Optional, llvm::cl::desc("Execute with Verbose output"),
                                   llvm::cl::cat(UserCat));
static llvm::cl::alias VerboseA("v", llvm::cl::NotHidden, llvm::cl::desc("Alias for --verbose"),
                                llvm::cl::aliasopt(Verbose));

static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);

void auto_argument_adjust(clang::tooling::ClangTool &tool)
{
    for (auto const &it : util::auto_include_dirs)
    {
        clang::tooling::ArgumentsAdjuster a = clang::tooling::getInsertArgumentAdjuster(it.c_str());
        tool.appendArgumentsAdjuster(a);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \brief vrtlmod main()
int main(int argc, const char **argv)
{
    int err = 0;

    llvm::Expected<clang::tooling::CommonOptionsParser> op =
        clang::tooling::CommonOptionsParser::create(argc, argv, UserCat);

    if (bool(Silent))
    {
        util::logging::toggle_silent();
        LOG_OBLIGAT("Executing silently - Warnings and Infos disabled");
    }

    if (bool(Verbose))
    {
        util::logging::toggle_verbose();
        LOG_VERBOSE("Executing verbosely - Verbose output active");
    }

    vrtlmod::VrtlmodCore core(OutputDir.c_str(), SystemC);

    if (bool(PrintTD))
    {
        core.print_targetdictionary();
        return 0;
    }

    if (!fs::exists(OutputDir.c_str()))
    {
        LOG_WARNING("Output directory ", OutputDir.c_str(), " doesn't exist! Will be created during execution.");
    }

    std::vector<std::string> in_sources = op->getSourcePathList();

    // prepare *.cpp /*.cc files: create, de-macro, clean comments.
    auto sources = core.prepare_sources(in_sources, Overwrite);

    // prepare *.h / *.hpp files: create, de-macro, clean comments.
    auto headers = core.prepare_headers(in_sources, Overwrite);

    auto srcs_and_headers = sources;
    srcs_and_headers.insert(srcs_and_headers.end(), headers.begin(), headers.end());
    auto srcs_and_headers_wo_symsh = srcs_and_headers;

    clang::tooling::ClangTool CommentTool(op->getCompilations(), srcs_and_headers);
    if (!bool(NoAutoInclude))
    {
        auto_argument_adjust(CommentTool);
    }
    LOG_INFO("Run CommentTool on sources ...");
    err = CommentTool.run(vrtlmod::CreateCommentRewritePass(core).get());
    LOG_INFO("... done");

    // create a new Clang Tool instance for Macro cleanup in source and header files except Verilated Symboltable header
    // which does not need cleanup, but breaks the MacroTool Lexer
    // FIXME: MacroTool only breaks for large VRTL models on Symboltable
    srcs_and_headers_wo_symsh.erase(std::remove_if(srcs_and_headers_wo_symsh.begin(), srcs_and_headers_wo_symsh.end(),
                                                   [](const auto &x)
                                                   { return (x.find("__Syms.h") != std::string::npos); }));
    clang::tooling::ClangTool MacroTool(op->getCompilations(), srcs_and_headers_wo_symsh);
    if (!bool(NoAutoInclude))
    {
        auto_argument_adjust(MacroTool);
    }
    LOG_INFO("Run MacroTool on sources ...");
    err = MacroTool.run(vrtlmod::CreateMacroRewritePass(core).get());
    LOG_INFO("... done");

    // postprocess *.h / *.hpp files: Remove anonymous structs
    LOG_INFO("Pre-process headers ...");
    core.preprocess_headers(headers);
    LOG_INFO("... done");

    clang::tooling::ClangTool stage1_ParserTool(op->getCompilations(), srcs_and_headers);
    if (!bool(NoAutoInclude))
    {
        auto_argument_adjust(stage1_ParserTool);
    }
    LOG_INFO("Analyze VRTL sources (elaboration)...");
    err = stage1_ParserTool.run(vrtlmod::CreateElaboratePass(core).get());
    LOG_INFO("... done");

    clang::tooling::ClangTool stage2_ParserTool(op->getCompilations(), srcs_and_headers);
    if (!bool(NoAutoInclude))
    {
        auto_argument_adjust(stage2_ParserTool);
    }
    LOG_INFO("Analyze VRTL sources for possible injection points ...");
    err = stage2_ParserTool.run(vrtlmod::CreateAnalyzePass(core).get());
    LOG_INFO("... done");

    core.build_xml();

    if (bool(XmlOnly))
        return 0;

    core.initialize_injection_targets(WhiteListXmlFilename);

    clang::tooling::ClangTool stage3_InjectionExprRewriterTool(op->getCompilations(), sources);
    if (!bool(NoAutoInclude))
    {
        auto_argument_adjust(stage3_InjectionExprRewriterTool);
    }
    LOG_INFO("Rewrite VRTL sources for injection points ...");
    err = stage3_InjectionExprRewriterTool.run(vrtlmod::CreateInjectionPass(core).get());
    LOG_INFO("... done");

    LOG_INFO("Rewrite VRTL headers for injectable signals ...");
    for (auto const &header_file : headers)
    {
        clang::tooling::ClangTool stage3_SigDeclRewriterTool(op->getCompilations(), { header_file });
        if (!bool(NoAutoInclude))
        {
            auto_argument_adjust(stage3_SigDeclRewriterTool);
        }
        err = stage3_SigDeclRewriterTool.run(vrtlmod::CreateSignalDeclPass(core).get());
    }
    LOG_INFO("... done");

    LOG_INFO("Generate API ...");
    core.build_api();
    LOG_INFO("... done");

    // postprocess *.h / *.hpp files: Reintroduce anonymous structs
    LOG_INFO("Postprocess modified sources ...");
    core.postprocess_headers(headers);
    LOG_INFO("... done");

    return err;
}
