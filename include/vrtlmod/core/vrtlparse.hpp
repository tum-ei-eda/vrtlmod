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
/// @file vrtlparse.hpp
/// @date Created on Mon Feb 07 11:45:11 2022
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_CORE_VRTLPARSE_HPP__
#define __VRTLMOD_CORE_VRTLPARSE_HPP__

#include "clang/AST/AST.h"
#include "clang/AST/ASTDumper.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Frontend/FrontendActions.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/Lexer.h"

#include "vrtlmod/core/consumer.hpp"
#include "vrtlmod/core/core.hpp"

#include <vector>

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all core vrtlmod functionalities
namespace vrtlmod
{
class VrtlmodPass;

////////////////////////////////////////////////////////////////////////////////
/// @class VrtlParser
/// @brief Abstract base that comes with matchers to parse a given VRTL code base
class VrtlParser : public Handler
{
    clang::SourceLocation last_seq_compound_begin_; ///< signals if we are currently in a sequent eval function's scope
    clang::SourceLocation last_seq_compound_end_;   ///< signals if we are currently in a sequent eval function's scope
    clang::ASTContext *last_seq_compound_ctx_{ nullptr }; ///< store context pointer to invalidate inter-file matching

    std::set<std::unique_ptr<VrtlmodPass>> passes_; ///< passes that extend match based action on parsed source code
  public:
    template <typename llvm_expr_t>
    std::string get_source_code_str(const llvm_expr_t *expr) const;

    template <typename llvm_expr_t>
    bool is_in_sequent(const llvm_expr_t *expr, const clang::ASTContext *ctx) const;

    template <typename llvm_expr_t>
    std::pair<const clang::MemberExpr *, const clang::CXXRecordDecl *> parse_sequential_assignment(
        const llvm_expr_t *expr, const clang::ast_matchers::MatchFinder::MatchResult &Result) const;

  public:
    void onEndOfTranslationUnit(void) override;
    ///////////////////////////////////////////////////////////////////////
    /// \brief ASTmatcher run. Implement this in the specific!
    virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Adds matchers
    virtual void addMatcher(clang::ast_matchers::MatchFinder &finder);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Add a new pass to this parse run
    void add_pass(std::unique_ptr<VrtlmodPass> pass);

  private:
    std::set<clang::SourceLocation> visited;

    std::set<std::pair<std::string, int>> func_macros_regex_paraidx_{ //>>>
                                                                      { "VL_CLEAN_WW", 2 },
                                                                      { "VL_ASSIGN_W", 1 },
                                                                      { "VL_ASSIGNBIT_II", 2 },
                                                                      { "VL_ASSIGNBIT_QI", 2 },
                                                                      { "VL_ASSIGNBIT_WI", 2 },
                                                                      { "VL_ASSIGNBIT_IO", 2 },
                                                                      { "VL_ASSIGNBIT_QO", 2 },
                                                                      { "VL_ASSIGNBIT_WO", 2 },
                                                                      { "VL_EXTEND_WI", 2 },
                                                                      { "VL_EXTEND_WQ", 2 },
                                                                      { "VL_EXTEND_WW", 2 },
                                                                      { "VL_EXTENDS_WI", 2 },
                                                                      { "VL_EXTENDS_WQ", 2 },
                                                                      { "VL_EXTENDS_WW", 2 },
                                                                      // Logical
                                                                      { "VL_AND_W", 1 },
                                                                      { "VL_OR_W", 1 },
                                                                      { "VL_XOR_W", 1 },
                                                                      { "VL_NOT_W", 1 },
                                                                      // Math
                                                                      { "VL_NEGATE_W", 1 },
                                                                      { "VL_NEGATE_INPLACE_W", 1 },
                                                                      { "VL_ADD_W", 1 },
                                                                      { "VL_SUB_W", 1 },
                                                                      { "VL_MUL_W", 1 },
                                                                      { "VL_MULS_W", 2 },
                                                                      { "VL_MULS_WWW", 2 },
                                                                      { "VL_DIVS_W", 1 },
                                                                      { "VL_DIVS_WWW", 1 },
                                                                      { "VL_MODDIVS_W", 1 },
                                                                      { "VL_MODDIVS_WWW", 1 },
                                                                      { "VL_POW_W", 3 },
                                                                      { "VL_POWSS_WWI", 3 },
                                                                      { "VL_POWSS_WWW", 3 },
                                                                      { "VL_POWSS_WWQ", 3 },
                                                                      // Concat/replication
                                                                      { "VL_REPLICATE_WII", 3 },
                                                                      { "VL_REPLICATE_WQI", 3 },
                                                                      { "VL_REPLICATE_WWI", 3 },
                                                                      { "VL_STREAML_WWI", 3 },
                                                                      { "VL_CONCAT_WII", 3 },
                                                                      { "VL_CONCAT_WWI", 3 },
                                                                      { "VL_CONCAT_WIW", 3 },
                                                                      { "VL_CONCAT_WIQ", 3 },
                                                                      { "VL_CONCAT_WQI", 3 },
                                                                      { "VL_CONCAT_WQQ", 3 },
                                                                      { "VL_CONCAT_WWQ", 3 },
                                                                      { "VL_CONCAT_WQW", 3 },
                                                                      { "VL_CONCAT_WWW", 3 },
                                                                      // Shifts
                                                                      { "VL_SHIFTL_WWI", 3 },
                                                                      { "VL_SHIFTL_WWW", 3 },
                                                                      { "VL_SHIFTL_WWQ", 3 },
                                                                      { "VL_SHIFTR_WWI", 3 },
                                                                      { "VL_SHIFTR_WWW", 3 },
                                                                      { "VL_SHIFTR_WWQ", 3 },
                                                                      { "VL_SHIFTRS_WWI", 3 },
                                                                      { "VL_SHIFTRS_WWW", 3 },
                                                                      { "VL_SHIFTRS_WWQ", 3 },
                                                                      // Bit selection
                                                                      { "VL_SEL_WWII", 4 },
                                                                      // Math needing insert/select
                                                                      { "VL_RTOIROUND_W_D", 1 },
                                                                      // Range assignments
                                                                      { "VL_ASSIGNSEL_WIII", 3 },
                                                                      { "VL_ASSIGNSEL_WIIQ", 3 },
                                                                      { "VL_ASSIGNSEL_WIIW", 3 },
                                                                      // Constification
                                                                      { "VL_COND_WIWW", 3 },
                                                                      { "VL_CONST_W_1X", 1 },
                                                                      { "VL_CONST_W_2X", 1 },
                                                                      { "VL_CONST_W_3X", 1 },
                                                                      { "VL_CONST_W_4X", 1 },
                                                                      { "VL_CONST_W_5X", 1 },
                                                                      { "VL_CONST_W_6X", 1 },
                                                                      { "VL_CONST_W_7X", 1 },
                                                                      { "VL_CONST_W_8X", 1 },
                                                                      { "VL_CONSTHI_W_1X", 2 },
                                                                      { "VL_CONSTHI_W_2X", 2 },
                                                                      { "VL_CONSTHI_W_3X", 2 },
                                                                      { "VL_CONSTHI_W_4X", 2 },
                                                                      { "VL_CONSTHI_W_5X", 2 },
                                                                      { "VL_CONSTHI_W_6X", 2 },
                                                                      { "VL_CONSTHI_W_7X", 2 },
                                                                      { "VL_CONSTHI_W_8X", 2 },
                                                                      { "VL_CONSTLO_W_8X", 1 }
    }; //<<< TODO: these Verilator functional macro references could be incomplete.

  public:
    std::set<const clang::FieldDecl *> found_instances_; ///< found instances of verilated modules in this parse run
    std::set<const clang::FieldDecl *> found_modules_;   ///< found verilated module declarations in this parse run
    std::set<const clang::FieldDecl *> found_signals_;   ///< found signal declarations in this parse run

    ///////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    /// \param cons Reference to consumer handler
    VrtlParser(Consumer &cons);
    ///////////////////////////////////////////////////////////////////////
    /// \brief Destructor
    virtual ~VrtlParser(void){};
};

///////////////////////////////////////////////////////////////////////
// template definitions:
template <typename llvm_expr_t>
std::string VrtlParser::get_source_code_str(const llvm_expr_t *expr) const
{
    return getRewriter().getRewrittenText(expr->getSourceRange());
}

template <typename llvm_expr_t>
bool VrtlParser::is_in_sequent(const llvm_expr_t *expr, const clang::ASTContext *ctx) const
{
    if ((ctx == last_seq_compound_ctx_) && (last_seq_compound_ctx_ != nullptr))
    {
        if (last_seq_compound_begin_.isValid() && last_seq_compound_end_.isValid())
        {
            if ((expr->getBeginLoc() >= last_seq_compound_begin_) && (expr->getEndLoc() <= last_seq_compound_end_))
            {
                return true;
            }
        }
    }
    return false;
}

template <typename llvm_expr_t>
std::pair<const clang::MemberExpr *, const clang::CXXRecordDecl *> VrtlParser::parse_sequential_assignment(
    const llvm_expr_t *expr, const clang::ast_matchers::MatchFinder::MatchResult &Result) const
{

    std::pair<const clang::MemberExpr *, const clang::CXXRecordDecl *> x{};
    auto &assignee = x.first;
    auto &parent = x.second;

    const clang::ASTContext *ctx = Result.Context;

    if (is_in_sequent(expr, ctx))
    {
        LOG_VERBOSE("{sea}:\n", "\\- ", get_source_code_str(expr), "\n",
                    util::logging::dump_to_str<const clang::Stmt *>(expr, ctx));

        if (const clang::MemberExpr *x = Result.Nodes.getNodeAs<clang::MemberExpr>("wsignal_field"))
        {
            LOG_VERBOSE("{wsignal_field}: ", get_source_code_str(x));
        }
        if (const clang::MemberExpr *x = Result.Nodes.getNodeAs<clang::MemberExpr>("signal"))
        {
            LOG_VERBOSE("{signal}: ", get_source_code_str(x));
            assignee = x;
        }
        if (const clang::MemberExpr *x = Result.Nodes.getNodeAs<clang::MemberExpr>("instance"))
        {
            parent = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("module");
            LOG_VERBOSE("{instance}: ", get_source_code_str(x), " of {module}:", parent->getNameAsString());
        }
        if (const clang::Expr *x = Result.Nodes.getNodeAs<clang::Expr>("symboltable"))
        {
            LOG_VERBOSE("{symboltable}: ", get_source_code_str(x));
        }
        if (const clang::Expr *x = Result.Nodes.getNodeAs<clang::Expr>("top"))
        {
            parent = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("module");
            LOG_VERBOSE("{top}: ", get_source_code_str(x), " of {module}:", parent->getNameAsString());
        }
        if (const clang::CXXThisExpr *x = Result.Nodes.getNodeAs<clang::CXXThisExpr>("this"))
        {
            parent = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("module");
            LOG_VERBOSE("{this}: ", get_source_code_str(x), "\n",
                        util::logging::dump_to_str<const clang::Stmt *>(x, ctx), "\nt:", x->getType().getAsString(),
                        " of {module}:", parent->getNameAsString());
        }
    }

    return x;
}

////////////////////////////////////////////////////////////////////////////////
/// @class ParserAction
/// @brief Frontend action for LLVM tool executing actions on the AST tree
template <typename pass_t>
class ParserAction : public clang::ASTFrontendAction
{
    clang::Rewriter rewriter_;
    std::string curfile;
    VrtlmodCore &core_;

  public:
    ParserAction(VrtlmodCore &core) : core_{ core } {}
    void EndSourceFileAction() { rewriter_.overwriteChangedFiles(); }

    // Add own Consumer and code rewriter to it
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef InFile);
};

template <typename pass_t>
std::unique_ptr<clang::ASTConsumer> ParserAction<pass_t>::CreateASTConsumer(clang::CompilerInstance &CI,
                                                                            llvm::StringRef InFile)
{
    curfile = InFile.str();
    rewriter_.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());

    auto cons = std::make_unique<Consumer>(rewriter_, curfile);
    auto pass = std::make_unique<pass_t>(core_);
    auto parser = std::make_unique<VrtlParser>(*cons);
    parser->add_pass(std::move(pass));

    cons->ownHandler(std::move(parser));

    return std::move(cons);
}

} // namespace vrtlmod

#endif // __VRTLMOD_PARSE_VRTLPARSE_HPP__
