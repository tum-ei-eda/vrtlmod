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
/// @file injectionrewrite.cpp
/// @date Created on Wed Mar 02 13:09:11 2022
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/passes/injectionrewrite.hpp"
#include "vrtlmod/core/types.hpp"

#include "vrtlmod/util/logging.hpp"

namespace vrtlmod
{
namespace passes
{
///////////////////////////////////////////////////////////////////////
/// \brief Returns String containing the intermitten injection statement
/// \param t Reference to Target
std::string get_synchronous_injection_stmt(const types::Target &t);
///////////////////////////////////////////////////////////////////////
/// \brief Returns String containing the sequential injection statement
/// \param t Reference to Target
/// \param subscripts Subscipts for array-based assignments. Empty vector if trivial
std::string get_sequent_injection_stmt(const types::Target &t, std::vector<std::string> subscripts);

void InjectionRewriter::action(const VrtlParser &parser,
                               const clang::ast_matchers::MatchFinder::MatchResult &Result) const
{
    auto ctx = Result.Context;

    if (const clang::FunctionDecl *f = Result.Nodes.getNodeAs<clang::FunctionDecl>("function"))
    {
        if (active_sequent_func_ != nullptr && active_sequent_func_ != f) // active_sequent_func_->func_ != f)
        {
            LOG_INFO("Leaving sequent function. Wrapping up: ", active_sequent_func_->getNameAsString());
            // LOG_INFO("Matcher ENTER sequent body of: \n\t", active_sequent_func_->_self());
            active_sequent_func_ = nullptr; // reset active
            active_compound_ = nullptr;
        }
    }

    if (const clang::FunctionDecl *f = Result.Nodes.getNodeAs<clang::FunctionDecl>("sequent_function_def"))
    {
        active_sequent_func_ = f;
        LOG_INFO("Matcher ENTER sequent body of: \n\t", active_sequent_func_->getNameAsString());
        map_seq_compounds_[active_sequent_func_] = {};
        map_injected_targets_[active_sequent_func_] = {};
        map_nonliteral_subscript_targets_[active_sequent_func_] = {};
    }

    if (const clang::Stmt *c = Result.Nodes.getNodeAs<clang::Stmt>("compound_of_sequent_func"))
    {
        if (const clang::FunctionDecl *f = Result.Nodes.getNodeAs<clang::FunctionDecl>("sequent_function_def"))
        {
            if (f != active_sequent_func_)
            {
                LOG_FATAL("Encountered compound statement [", std::to_string(c->getID(parser.getASTContext())),
                          "] is not child of active sequent function [", std::to_string(active_sequent_func_->getID()),
                          "]");
            }
            else
            {
                if (active_sequent_func_ != nullptr)
                {
                    auto compound = std::make_shared<CompoundStmt>(c, parser);
                    active_compound_ = compound.get();
                    map_seq_compounds_.at(active_sequent_func_).push_back(compound);
                }
                // TODO: maybe add here a locator to the needed sequent function to assign this compound to instead of
                // leaving on FATAL.
            }
        }
    }
    else if (const clang::Stmt *c = Result.Nodes.getNodeAs<clang::Stmt>("compound"))
    {
        if (parser.is_in_sequent(c, ctx))
        {
            if (active_sequent_func_ != nullptr)
            {
                auto compound = std::make_shared<CompoundStmt>(c, parser);
                active_compound_ = compound.get();
                map_seq_compounds_.at(active_sequent_func_).push_back(compound);
            }
        }
    }

    auto check_add_seqinjection = [&](const auto &expr) -> const types::Target *
    {
        auto assignee_parent_pair = parser.parse_sequential_assignment(expr, Result);

        auto assignee = assignee_parent_pair.first;
        auto parent = assignee_parent_pair.second;

        if (assignee != nullptr && parent != nullptr)
        {
            LOG_VERBOSE("INJREW: check target status of: `{signal} ", parser.get_source_code_str(assignee), " aka `",
                        assignee->getMemberNameInfo().getAsString(), "` of `{module}:", parent->getNameAsString(), "`");

            auto target_index = get_core().is_expr_target(assignee, parent, *ctx);
            if (target_index >= 0)
            {
                auto &t = get_core().get_target_from_index(target_index);
                if (t.is_assigned_here(assignee, parent))
                {
                    LOG_INFO("INJREW: injection location for [", t.get_id(), "] found assignment:\n  \\- ",
                             parser.get_source_code_str(expr), " at: ", t.get_inj_loc());
                    LOG_VERBOSE("AST:\n", util::logging::dump_to_str<const clang::Stmt *>(expr, ctx));
                    return &t;
                }
            }
        }
        return nullptr;
    };

    std::string prefix = "";
    if (const clang::MemberExpr *instance = Result.Nodes.getNodeAs<clang::MemberExpr>("instance"))
    {
        prefix = parser.getRewriter().getRewrittenText(instance->getSourceRange());
        prefix += ".";
    }
    else if (const clang::Expr *top = Result.Nodes.getNodeAs<clang::Expr>("top"))
    {
        prefix = parser.getRewriter().getRewrittenText(top->getSourceRange());
        prefix += "->";
    }
    else if (const clang::CXXThisExpr *x = Result.Nodes.getNodeAs<clang::CXXThisExpr>("this"))
    {
        prefix = "this->";
    }

    auto is_integer_literal = [&](const auto &expr) -> bool
    {
        if (auto *cast = llvm::dyn_cast<clang::ImplicitCastExpr>(expr))
        {
            if (auto *integerliteral = llvm::dyn_cast<clang::IntegerLiteral>(cast->getSubExpr()))
            {
                return true;
            }
        }
        return false;
    };

    if (prefix != "")
    {
        // if (const clang::BinaryOperator *x = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea"))
        {
            bool has_non_literal = false;
            const types::Target *t = nullptr;
            std::shared_ptr<BinarySInj> asgn{ nullptr };

            if (const auto *x = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_array_oop_oop_3d"))
            {
                // overloaded [] operator lhs
                // we need to get and check if more than on {signal} was matching here.
                LOG_INFO("{sea_binary_array_oop_oop_3d}:", util::logging::dump_to_str<const clang::Stmt *>(x, ctx));
                if (const auto *narrow = Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("oop_1d"))
                {
                    LOG_INFO("{oop_1d}:", util::logging::dump_to_str<const clang::Stmt *>(narrow, ctx));
                    auto idx_1d = narrow->getArg(1);
                    has_non_literal |= !is_integer_literal(idx_1d);
                    if (const auto *oop_2d = Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("oop_2d"))
                    {
                        LOG_INFO("{oop_2d}:", util::logging::dump_to_str<const clang::Stmt *>(oop_2d, ctx));
                        auto idx_2d = oop_2d->getArg(1);
                        has_non_literal |= !is_integer_literal(idx_2d);
                        if (const auto *array_3d = Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("array_3d"))
                        {
                            LOG_INFO("{array_3d}:", util::logging::dump_to_str<const clang::Stmt *>(array_3d, ctx));
                            auto idx_3d = array_3d->getIdx();
                            has_non_literal |= !is_integer_literal(idx_3d);
                            t = check_add_seqinjection(narrow);
                            if (t != nullptr)
                            {
                                auto _asgn = std::make_shared<BinarySubscriptedSInj>(narrow->getArg(0), x, prefix, t);
                                _asgn->set_index(0, idx_1d);
                                _asgn->set_index(1, idx_2d);
                                _asgn->set_index(2, idx_3d);
                                asgn = _asgn;
                            }
                        }
                        else
                        {
                            LOG_FATAL(
                                "Could not find expected {array_3d} match for matched {sea_binary_array_oop_oop_3d}!");
                        }
                    }
                    else
                    {
                        LOG_FATAL("Could not find expected {oop_2d} match for matched {sea_binary_array_oop_oop_3d}!");
                    }
                }
                else
                {
                    LOG_FATAL("Could not find expected {oop_1d} match for matched {sea_binary_array_oop_oop_3d}!");
                }
            }
            else if (const auto *x = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_array_oop_2d"))
            {
                // overloaded [] operator lhs
                // we need to get and check if more than on {signal} was matching here.
                LOG_INFO("{sea_binary_array_oop_2d}:", util::logging::dump_to_str<const clang::Stmt *>(x, ctx));
                if (const auto *narrow = Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("oop_1d"))
                {
                    LOG_INFO("{oop_1d}:", util::logging::dump_to_str<const clang::Stmt *>(narrow, ctx));
                    auto idx_1d = narrow->getArg(1);
                    has_non_literal |= !is_integer_literal(idx_1d);
                    if (const auto *array_2d = Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("array_2d"))
                    {
                        LOG_INFO("{array_2d}:", util::logging::dump_to_str<const clang::Stmt *>(array_2d, ctx));
                        auto idx_2d = array_2d->getIdx();
                        has_non_literal |= !is_integer_literal(idx_2d);
                        t = check_add_seqinjection(narrow);
                        if (t != nullptr)
                        {
                            auto _asgn = std::make_shared<BinarySubscriptedSInj>(narrow->getArg(0), x, prefix, t);
                            _asgn->set_index(0, idx_1d);
                            _asgn->set_index(1, idx_2d);
                            asgn = _asgn;
                        }
                    }
                    else
                    {
                        LOG_FATAL("Could not find expected {array_2d} match for matched {sea_binary_array_oop_2d}!");
                    }
                }
                else
                {
                    LOG_FATAL("Could not find expected {oop_1d} match for matched {sea_binary_array_oop_2d}!");
                }
            }
            else if (const auto *x = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_oop_3d"))
            {
                // overloaded [] operator lhs
                // we need to get and check if more than on {signal} was matching here.
                LOG_INFO("{sea_binary_oop_3d}:", util::logging::dump_to_str<const clang::Stmt *>(x, ctx));
                if (const auto *narrow = Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("oop_1d"))
                {
                    LOG_INFO("{oop_1d}:", util::logging::dump_to_str<const clang::Stmt *>(narrow, ctx));
                    auto idx_1d = narrow->getArg(1);
                    has_non_literal |= !is_integer_literal(idx_1d);
                    if (const auto *oop_2d = Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("oop_2d"))
                    {
                        LOG_INFO("{oop_2d}:", util::logging::dump_to_str<const clang::Stmt *>(oop_2d, ctx));
                        auto idx_2d = oop_2d->getArg(1);
                        has_non_literal |= !is_integer_literal(idx_2d);
                        if (const auto *oop_3d = Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("oop_3d"))
                        {
                            LOG_INFO("{oop_3d}:", util::logging::dump_to_str<const clang::Stmt *>(oop_3d, ctx));
                            auto idx_3d = oop_3d->getArg(1);
                            has_non_literal |= !is_integer_literal(idx_3d);
                            t = check_add_seqinjection(narrow);
                            if (t != nullptr)
                            {
                                auto _asgn = std::make_shared<BinarySubscriptedSInj>(narrow->getArg(0), x, prefix, t);
                                _asgn->set_index(0, idx_1d);
                                _asgn->set_index(1, idx_2d);
                                _asgn->set_index(2, idx_3d);
                                asgn = _asgn;
                            }
                        }
                        else
                        {
                            LOG_FATAL("Could not find expected {oop_3d} match for matched {sea_binary_oop_3d}!");
                        }
                    }
                    else
                    {
                        LOG_FATAL("Could not find expected {oop_2d} match for matched {sea_binary_oop_3d}!");
                    }
                }
                else
                {
                    LOG_FATAL("Could not find expected {oop_1d} match for matched {sea_binary_oop_3d}!");
                }
            }
            else if (const auto *x = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_oop_2d"))
            {
                // overloaded [] operator lhs
                // we need to get and check if more than on {signal} was matching here.
                LOG_INFO("{sea_binary_oop_2d}:", util::logging::dump_to_str<const clang::Stmt *>(x, ctx));
                if (const auto *narrow = Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("oop_1d"))
                {
                    LOG_INFO("{oop_1d}:", util::logging::dump_to_str<const clang::Stmt *>(narrow, ctx));
                    auto idx_1d = narrow->getArg(1);
                    has_non_literal |= !is_integer_literal(idx_1d);
                    if (const auto *oop_2d = Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("oop_2d"))
                    {
                        LOG_INFO("{oop_2d}:", util::logging::dump_to_str<const clang::Stmt *>(oop_2d, ctx));
                        auto idx_2d = oop_2d->getArg(1);
                        has_non_literal |= !is_integer_literal(idx_2d);
                        t = check_add_seqinjection(narrow);
                        if (t != nullptr)
                        {
                            auto _asgn = std::make_shared<BinarySubscriptedSInj>(narrow->getArg(0), x, prefix, t);
                            _asgn->set_index(0, idx_1d);
                            _asgn->set_index(1, idx_2d);
                            asgn = _asgn;
                        }
                    }
                    else
                    {
                        LOG_FATAL("Could not find expected {oop_2d} match for matched {sea_binary_oop_2d}!");
                    }
                }
                else
                {
                    LOG_FATAL("Could not find expected {oop_1d} match for matched {sea_binary_oop_2d}!");
                }
            }
            else if (const auto *x = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_oop_1d"))
            {
                // overloaded [] operator lhs
                // we need to get and check if more than on {signal} was matching here.
                LOG_INFO("{sea_binary_oop_1d}:", util::logging::dump_to_str<const clang::Stmt *>(x, ctx));
                if (const auto *narrow = Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("oop_1d"))
                {
                    LOG_INFO("{oop_1d}:", util::logging::dump_to_str<const clang::Stmt *>(narrow, ctx));
                    auto idx_1d = narrow->getArg(1);
                    has_non_literal |= !is_integer_literal(idx_1d);
                    t = check_add_seqinjection(narrow);
                    if (t != nullptr)
                    {
                        auto _asgn = std::make_shared<BinarySubscriptedSInj>(narrow->getArg(0), x, prefix, t);
                        _asgn->set_index(0, idx_1d);
                        asgn = _asgn;
                    }
                }
                else
                {
                    LOG_FATAL("Could not find expected {oop} match for matched {sea_binary_oop_1d}!");
                }
            }
            else if (const auto *x = Result.Nodes.getNodeAs<clang::BinaryOperator>(
                         "sea_binary_array_3d")) // start matching with multidimensional arrays, otherwise it could
                                                 // happen that a target that is a subscript for a multidimensional
                                                 // array assignment is identified in the AST before the base of the
                                                 // multidim.
            {
                // array subscript
                LOG_INFO("{sea_binary_array_3d}:", util::logging::dump_to_str<const clang::Stmt *>(x, ctx));
                if (const auto *narrow = Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("array"))
                {
                    LOG_INFO("{array_1d}:", util::logging::dump_to_str<const clang::Stmt *>(narrow, ctx));
                    auto idx_1d = narrow->getIdx();
                    has_non_literal |= !is_integer_literal(idx_1d);
                    if (const auto *arr_2d = Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("array_2d"))
                    {
                        LOG_INFO("{array_2d}:", util::logging::dump_to_str<const clang::Stmt *>(arr_2d, ctx));
                        auto idx_2d = arr_2d->getIdx();
                        has_non_literal |= !is_integer_literal(idx_2d);
                        if (const auto *arr_3d = Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("array_3d"))
                        {
                            LOG_INFO("{array_3d}:", util::logging::dump_to_str<const clang::Stmt *>(arr_3d, ctx));
                            auto idx_3d = arr_3d->getIdx();
                            has_non_literal |= !is_integer_literal(idx_3d);
                            t = check_add_seqinjection(narrow);
                            if (t != nullptr)
                            {
                                auto _asgn = std::make_shared<BinarySubscriptedSInj>(narrow->getBase(), x, prefix, t);
                                _asgn->set_index(0, idx_1d);
                                _asgn->set_index(1, idx_2d);
                                _asgn->set_index(2, idx_3d);
                                asgn = _asgn;
                            }
                        }
                        else
                        {
                            LOG_FATAL("Could not find expected {array_3d} match for matched {sea_binary_array_3d}!");
                        }
                    }
                    else
                    {
                        LOG_FATAL("Could not find expected {array_2d} match for matched {sea_binary_array_3d}!");
                    }
                }
                else
                {
                    LOG_FATAL("Could not find expected {array_1d} match for matched {sea_binary_array_3d}!");
                }
            }
            else if (const auto *x = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_array_2d"))
            {
                // array subscript
                LOG_INFO("{sea_binary_array_2d}:", util::logging::dump_to_str<const clang::Stmt *>(x, ctx));
                if (const auto *narrow = Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("array_1d"))
                {
                    LOG_INFO("{array_1d}:", util::logging::dump_to_str<const clang::Stmt *>(narrow, ctx));
                    auto idx_1d = narrow->getIdx();
                    has_non_literal |= !is_integer_literal(idx_1d);
                    if (const auto *arr_2d = Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("array_2d"))
                    {
                        LOG_INFO("{array_2d}:", util::logging::dump_to_str<const clang::Stmt *>(arr_2d, ctx));
                        auto idx_2d = arr_2d->getIdx();
                        has_non_literal |= !is_integer_literal(idx_2d);
                        t = check_add_seqinjection(narrow);

                        if (t != nullptr)
                        {
                            auto _asgn = std::make_shared<BinarySubscriptedSInj>(narrow->getBase(), x, prefix, t);
                            _asgn->set_index(0, idx_1d);
                            _asgn->set_index(1, idx_2d);
                            asgn = _asgn;
                        }
                    }
                    else
                    {
                        LOG_FATAL("Could not find expected {array_2d} match for matched {sea_binary_array_2d}!");
                    }
                }
                else
                {
                    LOG_FATAL("Could not find expected {array_1d} match for matched {sea_binary_array_2d}!");
                }
            }
            else if (const auto *x = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_array_1d"))
            {
                // array subscript
                LOG_INFO("{sea_binary_array_1d}:", util::logging::dump_to_str<const clang::Stmt *>(x, ctx));
                if (const auto *narrow = Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("array_1d"))
                {
                    LOG_INFO("{array_1d}:", util::logging::dump_to_str<const clang::Stmt *>(narrow, ctx));
                    auto idx_1d = narrow->getIdx();
                    has_non_literal |= !is_integer_literal(idx_1d);
                    t = check_add_seqinjection(narrow);
                    if (t != nullptr)
                    {
                        auto _asgn = std::make_shared<BinarySubscriptedSInj>(narrow->getBase(), x, prefix, t);
                        _asgn->set_index(0, idx_1d);
                        asgn = _asgn;
                    }
                }
                else
                {
                    LOG_FATAL("Could not find expected {array_1d} match for matched {sea_binary_array_1d}!");
                }
            }
            else if (const clang::BinaryOperator *x =
                         Result.Nodes.getNodeAs<clang::BinaryOperator>("sea_binary_trivial"))
            {
                if (t != nullptr)
                {
                    LOG_FATAL("We have already matched another binary assignment matcher!");
                }
                // native/trivial sequential assignment
                t = check_add_seqinjection(x->getLHS());
                if (t != nullptr)
                {
                    asgn = std::make_shared<BinarySInj>(x, prefix, t);
                }
            }

            if (asgn.get() != nullptr && active_compound_ != nullptr)
            {
                auto comp = get_finest_compound(active_sequent_func_, asgn->get_base_expr());
                comp->add_assignment(asgn);
                map_injected_targets_.at(active_sequent_func_).insert({ prefix, t });
                if (has_non_literal)
                {
                    map_nonliteral_subscript_targets_.at(active_sequent_func_).insert({ prefix, t });
                }
            }
        }
        if (const clang::CallExpr *x = Result.Nodes.getNodeAs<clang::CallExpr>("sea_func"))
        {
            LOG_VERBOSE("{sea_func}: [", x->getDirectCallee()->getNameAsString(), "] ",
                        util::logging::dump_to_str<const clang::Stmt *>(x, ctx));
            if (const clang::Expr *arg = Result.Nodes.getNodeAs<clang::Expr>("arg1_expr"))
            {
                LOG_VERBOSE("{arg1_expr}: ", util::logging::dump_to_str<const clang::Stmt *>(arg, ctx));
                if (const types::Target *t = check_add_seqinjection(x))
                {
                    // TODO: function call assigning new values to arg1_expr, revert back to synchronous injection,
                    // b.c. hard to deduce assigned bits. This might need a thorough rework.
                    auto assignee_parent_pair = parser.parse_sequential_assignment(arg, Result);
                    auto assignee = assignee_parent_pair.first;
                    auto parent = assignee_parent_pair.second;

                    if (assignee != nullptr && parent != nullptr)
                    {
                        auto asgn = std::make_shared<CallSInj>(x, assignee, prefix, t);
                        auto comp = get_finest_compound(active_sequent_func_, asgn->get_base_expr());
                        // active_compound_->add_assignment(asgn);
                        comp->add_assignment(asgn);
                        map_injected_targets_.at(active_sequent_func_).insert({ prefix, t });
                    }
                }
            }
        }
    }
}

std::vector<std::shared_ptr<InjectionRewriter::SInj>> InjectionRewriter::CompoundStmt::get_dominant_assignments(
    void) const
{
    std::vector<std::shared_ptr<InjectionRewriter::SInj>> ret{};

    for (auto const &outer : asngs_)
    {
        std::string outer_lhsstr =
            parser_.getRewriter().getRewrittenText(outer->get_base_expr()->getSourceRange()).c_str();
        auto outer_begin = outer->get_base_expr()->getBeginLoc();
        bool found = false;

        for (auto &inner : ret)
        {
            auto inner_begin = inner->get_base_expr()->getBeginLoc();
            if (inner_begin < outer_begin)
            {
                std::string inner_lhsstr =
                    parser_.getRewriter().getRewrittenText(inner->get_base_expr()->getSourceRange()).c_str();
                if (outer_lhsstr == inner_lhsstr)
                {
                    inner = outer; // replace the last occurence with this latest one (dominant in sequential compound.)
                    found = true;
                    break;
                }
            }
        }
        if (!found)
            ret.push_back(outer);
    }
    return ret;
}

void InjectionRewriter::BinarySubscriptedSInj::rewrite_injection(const VrtlParser &parser, bool write_as_comment) const
{
    BinarySInj::rewrite_injection(parser, write_as_comment);
}

void InjectionRewriter::BinarySInj::rewrite_injection(const VrtlParser &parser, bool write_as_comment) const
{
    int idx;
    LOG_INFO("op: --- ", parser.getRewriter().getRewrittenText(expr_->getSourceRange()).c_str());

    std::string name, s, lhs = parser.getRewriter().getRewrittenText(expr_->getLHS()->getSourceRange());
    std::vector<std::string> subscripts{};

    size_t bropen = lhs.find('[');
    if (bropen != std::string::npos)
    {
        name = lhs.substr(0, bropen);
        size_t lastmajor_open = 0, lastmajor_close = 0;
        size_t pos = 0;
        int brcnt = 0;
        for (char const &c : lhs)
        {
            if (c == '[')
            {
                if (brcnt == 0)
                {
                    lastmajor_open = pos;
                }
                ++brcnt;
            }
            if (c == ']')
            {
                --brcnt;
                if (brcnt == 0)
                {
                    lastmajor_close = pos;
                    subscripts.push_back(lhs.substr(lastmajor_open + 1, lastmajor_close - (lastmajor_open + 1)));
                }
            }
            ++pos;
        }
    }

    std::string str = parser.getRewriter().getRewrittenText(expr_->getSourceRange());
    str += "; ";
    if (write_as_comment)
    {
        str += "// ";
        str += prefix_;
        auto sis_str = get_sequent_injection_stmt(*t_, subscripts);
        util::strhelp::replaceAll(sis_str, "\n", "\n// ");
        str += sis_str;
    }
    else
    {
        str += prefix_;
        str += get_sequent_injection_stmt(*t_, subscripts);
    }

    LOG_INFO("Writing sequential injection point [", str, "] for target: ", t_->_self());
    parser.getRewriter().ReplaceText(expr_->getSourceRange(), str);
}

void InjectionRewriter::CallSInj::rewrite_injection(const VrtlParser &parser, bool write_as_comment) const
{
    std::string str = parser.getRewriter().getRewrittenText(expr_->getSourceRange());
    str += "; ";
    if (write_as_comment)
    {
        str += "// ";
        str += prefix_;
        auto sis_str = get_synchronous_injection_stmt(*t_);
        util::strhelp::replaceAll(sis_str, "\n", "\n// ");
        str += sis_str;
    }
    else
    {
        str += prefix_;
        str += get_synchronous_injection_stmt(*t_);
    }

    parser.getRewriter().ReplaceText(expr_->getSourceRange(), str);
}

void InjectionRewriter::wrap_up_sequent_function(const clang::FunctionDecl *func, const VrtlParser &parser) const
{
    std::stringstream insert;
    insert.str("");
    insert.clear();

    auto &ts = map_injected_targets_.at(func);
    LOG_INFO("INJREW Adding non-dominant injection points to end of sequential function:", func->getNameAsString());

    std::string newc = parser.getRewriter().getRewrittenText(func->getSourceRange());

    LOG_VERBOSE("{compset}: of func ", func->getNameAsString());
    insert << std::endl << "    //>>> vRTLmod non-dominant target injections" << std::endl;

    for (auto const &it : ts)
    {
        LOG_VERBOSE("\\-> target ", it.second->_self());
        insert << "    " << it.first << get_synchronous_injection_stmt(*(it.second)) << ";" << std::endl;
    }
    insert << "    //<<< VRTLFI non-dominant target injections" << std::endl;
    insert << "}";

    // find last '}' as function body end
    auto posLastBr = newc.rfind("}");
    if (posLastBr != std::string::npos)
    {
        newc.erase(posLastBr);
        newc.append(insert.str());
        parser.getRewriter().ReplaceText(func->getSourceRange(), newc);
    }
}

InjectionRewriter::InjectionRewriter(const VrtlmodCore &core) : VrtlmodPass(core) {}

InjectionRewriter::~InjectionRewriter(void) {}

std::string get_synchronous_injection_stmt(const types::Target &t)
{
    std::string str = util::concat(t.get_id(), "__td_->inject_synchronous()");

    return str;
}

void InjectionRewriter::end_of_translation(const VrtlParser &parser) const
{
    write_sequent_injections();
    for (const auto &[func, targets] : map_injected_targets_)
    {
        wrap_up_sequent_function(func, parser);
    }
}

void InjectionRewriter::write_sequent_injections(void) const
{
    for (auto const &[seq_func, seq_compounds] : map_seq_compounds_)
    {
        for (auto const &compound : seq_compounds)
        {
            auto dominant_asgns = compound->get_dominant_assignments();
            for (auto const sinj : dominant_asgns)
            {
                // find target in map_nonliteral_subscript_targets_, if so skip sequential injection
                bool skip = false;
                auto &ts = map_nonliteral_subscript_targets_.at(seq_func);
                for (auto const &[prefix, target] : ts)
                {
                    skip |= (target == sinj->t_) && (prefix == sinj->prefix_);
                    if (skip)
                    {
                        break;
                    }
                }
                sinj->rewrite_injection(compound->parser_, skip);
            }
        }
    }
}

std::shared_ptr<InjectionRewriter::CompoundStmt> InjectionRewriter::get_finest_compound(const clang::FunctionDecl *f,
                                                                                        const clang::Expr *expr) const
{
    std::shared_ptr<CompoundStmt> ret{ nullptr };
    auto in_range = [](const clang::Expr *a, const auto *b) -> bool
    {
        if ((a->getBeginLoc().getRawEncoding() >= b->getBeginLoc().getRawEncoding()) &&
            (a->getEndLoc().getRawEncoding() <= b->getEndLoc().getRawEncoding()))
        {
            return true;
        }
        return false;
    };

    for (auto &comp : map_seq_compounds_.at(f))
    {
        if (in_range(expr, comp->c_))
        {
            if (!ret)
            {
                ret = comp; // none found yet
            }
            else
            {
                if (ret->c_->getBeginLoc() < comp->c_->getBeginLoc())
                {
                    ret = comp; // one more fine grain comp found
                }
            }
        }
    }

    return ret;
}

std::string get_sequent_injection_stmt(const types::Target &t, std::vector<std::string> subscripts)
{
    std::string str = util::concat(t.get_id(), "__td_->__inject_on_update(");

    if (subscripts.size() > 0)
    {
        bool first = true;
        for (const auto &it : subscripts)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                str += ", ";
            }
            str += it;
        }
    }
    str += ")";

    return str;
}

} // namespace passes
} // namespace vrtlmod
