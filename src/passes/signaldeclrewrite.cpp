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
/// @file signaldeclrewrite.cpp
/// @date Created on Wed Feb 02 17:46:20 2022
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/passes/signaldeclrewrite.hpp"
#include "vrtlmod/core/types.hpp"

#include "vrtlmod/util/logging.hpp"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

namespace vrtlmod
{
namespace passes
{

void SignalDeclRewriter::action(const VrtlParser &parser,
                                const clang::ast_matchers::MatchFinder::MatchResult &Result) const
{
    auto ctx = Result.Context;
    auto &srcmgr = ctx->getSourceManager();
    auto &lang_opts = ctx->getLangOpts();

    auto get_source_code_str = [&](const auto *x)
    { return parser.getRewriter().getRewrittenText(x->getSourceRange()); };

    if (const clang::FieldDecl *x = Result.Nodes.getNodeAs<clang::FieldDecl>("signal_decl"))
    {
        LOG_VERBOSE("{signal_decl}: ", x->getNameAsString(), "\n  `\\-", get_source_code_str(x));
        LOG_VERBOSE("AST:\n", util::logging::dump_to_str(x));

        auto target_index = get_core().is_decl_target(x);
        if (target_index >= 0)
        {
            auto &t = get_core().get_target_from_index(target_index);
            if (t.check_declared_here(x))
            {
                LOG_VERBOSE(">found injection target declaration: ", t._self());
                if (t.has_rewritten_decl() == false)
                {
                    LOG_INFO("Rewrite declaration of target: ", t._self());
                    write_injection_decl(x, t, parser);
                    t.decl_rewritten_ = true;
                }
            }
        }
    }
}

void SignalDeclRewriter::write_injection_decl(const clang::FieldDecl *decl, const types::Target &t,
                                              const VrtlParser &parser) const
{
    std::stringstream x;
    x << parser.getRewriter().getRewrittenText(decl->getSourceRange()) << "; ";
    x << "vrtlfi::td::";

    auto cxxdim = t.get_cxx_dimension_lengths();
    auto cxxdimtypes = t.get_cxx_dimension_types();

    switch (cxxdim.size())
    {
    case 0:
        x << "ZeroD_TDentry<" << t.get_cxx_type() << ">"
          << " *" << t.get_id() << "__td_";
        break;
    case 1:
        x << "OneD_TDentry<" << t.get_cxx_type() << ", " << cxxdimtypes.back() << ", " << cxxdim[0] << ">"
          << " *" << t.get_id() << "__td_";
        break;
    case 2:
        x << "TwoD_TDentry<" << t.get_cxx_type() << ", " << cxxdimtypes.back() << ", " << cxxdim[0] << ", " << cxxdim[1]
          << ">"
          << " *" << t.get_id() << "__td_";
        break;
    case 3:
        x << "ThreeD_TDentry<" << t.get_cxx_type() << ", " << cxxdimtypes.back() << ", " << cxxdim[0] << ", "
          << cxxdim[1] << ", " << cxxdim[2] << ">"
          << " *" << t.get_id() << "__td_";
        break;
    default:
        LOG_ERROR("CType dimensions of injection target not supported: ", t.get_cxx_type());
        break;
    }

    parser.getRewriter().ReplaceText(decl->getSourceRange(), x.str());
    modify_includes(decl, parser);
}

void SignalDeclRewriter::modify_includes(const clang::Decl *decl, const VrtlParser &parser) const
{
    auto& srcmgr = parser.getRewriter().getSourceMgr();
    static std::set<const FileEntry*> modified_fentries{};
    
    FileID fid = srcmgr.getFileID(decl->getBeginLoc());
    const FileEntry* fentry = srcmgr.getFileEntryForID(fid);
    if (modified_fentries.find(fentry) != modified_fentries.end())
    {
        LOG_VERBOSE(">modify includes: file[", fentry->getName().str(), "] already modified. skipping.");
        return; // already modified
    }

    SourceLocation flocSOF = srcmgr.getLocForStartOfFile(fid);
    SourceRange flocra = SourceRange(flocSOF, srcmgr.getLocForEndOfFile(fid));

    std::string newc = parser.getRewriter().getRewrittenText(flocra);
    auto includepos = newc.find("#include");
    if (includepos != std::string::npos)
    {
        LOG_VERBOSE(">modify includes: file[", fentry->getName().str(), "] at pos[", std::to_string(includepos), "]");
        std::string x = get_core().get_include_string();
        if (newc.find(x) == std::string::npos) // do not insert if already existing
        {
            newc.insert(includepos, x);
            // parser.getRewriter().InsertTextBefore(flocSOF, get_core().getTDExternalDecl()); // todo this was
            // previously used for the global singleton, we no longer need because injection points are incorporated in
            // the VRTL class definition
            parser.getRewriter().ReplaceText(flocra, newc);
            modified_fentries.insert(fentry);
        }
    }
}

SignalDeclRewriter::SignalDeclRewriter(const VrtlmodCore &core) : VrtlmodPass(core) {}

void SignalDeclRewriter::analyzeRewrite(void)
{
    auto func = [&](const types::Target &it) { return true; };
    get_core().foreach_injection_target(func);
}

SignalDeclRewriter::~SignalDeclRewriter(void)
{
    analyzeRewrite();
}

} // namespace passes
} // namespace vrtlmod
