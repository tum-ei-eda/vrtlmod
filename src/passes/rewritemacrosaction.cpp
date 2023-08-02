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

void RewriteCommentsAction::ExecuteAction(void)
{
    LOG_VERBOSE("> Rewrite inlined (/* */) comments within comments (//) in file", getCurrentFile().str());

    clang::CompilerInstance &CI = getCompilerInstance();
    clang::Preprocessor &PP = CI.getPreprocessor();
    clang::SourceManager &SM = PP.getSourceManager();
    clang::Rewriter Rewrite;
    Rewrite.setSourceMgr(SM, PP.getLangOpts());
    clang::RewriteBuffer &RB = Rewrite.getEditBuffer(SM.getMainFileID());

    ch_.set_file(getCurrentFile());
    ch_.set_rewrite_buffer(RB);
    CI.getPreprocessor().addCommentHandler(&ch_);

    llvm::raw_null_ostream null_out;
    RewriteMacrosInInput(CI.getPreprocessor(), &null_out);

    std::ofstream out(getCurrentFile().str().c_str());
    out << std::string(RB.begin(), RB.end());
    out.flush();
    out.close();
    CI.getPreprocessor().removeCommentHandler(&ch_);
}

bool RewriteCommentsAction::CommentHandler::HandleComment(clang::Preprocessor &PP, clang::SourceRange Comment)
{
    clang::SourceManager &sm = PP.getSourceManager();
    if (sm.getFilename(Comment.getBegin()) == file_)
    {
        auto comment_loc_str = Comment.printToString(sm);
        std::pair<clang::FileID, unsigned int> startLoc = sm.getDecomposedLoc(Comment.getBegin());
        std::pair<clang::FileID, unsigned int> endLoc = sm.getDecomposedLoc(Comment.getEnd());

        llvm::StringRef fileData = sm.getBufferData(startLoc.first);

        auto comment_str = fileData.substr(startLoc.second, endLoc.second - startLoc.second).str();

        // remove multi-line comments (`/**/`) inlined in single-line comments (`//`)`, e.g., // ... /* some text */ ...
        // clang::RewriteMacrosAction::RewriteMacrosInInput can not detect those and would result in /* // ... /*
        // some text */ ...  */ which breaks preprocessor at first /**/ pair
        if (auto inline_pos_start = comment_str.find("/*", 2); inline_pos_start != std::string::npos)
        {
            clang::Rewriter rw;
            rw.setSourceMgr(sm, PP.getLangOpts());
            clang::RewriteBuffer &RB = rw.getEditBuffer(sm.getMainFileID());

            std::cout << ">>>[c]: was `" << comment_str << "`" << std::endl;
            std::string new_str;
            new_str = comment_str.substr(2);
            util::strhelp::replaceAll(new_str, "/*", "\\/\\*");
            util::strhelp::replaceAll(new_str, "*/", "\\*\\/");
            if (comment_str.find("/*") == 0) // original comment embedding another inline comment is itself an inline
                                             // comment (may happen if RewriteMacrosAction had been already done)
            {
                new_str += "*/";
            }
            new_str = comment_str.substr(0, 2) + new_str;
            LOG_INFO("Rewriting multi-line comments (`/**/`) inlined in single-line comments (`//`)`: from [", new_str, "] to [",  "]");

            rb_->ReplaceText(sm.getFileOffset(Comment.getBegin()), comment_str.size(), new_str);
        }
    }
    return false;
}

} // namespace rewrite
} // namespace transform
} // namespace vrtlmod
