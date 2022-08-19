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
        if (active_sequent_func_ != nullptr && active_sequent_func_ != f)
        {
            LOG_INFO("Leaving sequent function. Wrapping up: ", active_sequent_func_->getNameAsString());
            active_sequent_func_ = nullptr; // reset active
        }
    }

    if (const clang::FunctionDecl *f = Result.Nodes.getNodeAs<clang::FunctionDecl>("sequent_function_def"))
    {
        active_sequent_func_ = f;
        map_injected_targets_[active_sequent_func_] = {};
        map_sequential_injections_[active_sequent_func_] = {};
        LOG_INFO("Matcher ENTER sequent body of: \n\t", active_sequent_func_->getNameAsString());
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

    if (prefix != "")
    {
        if (const clang::BinaryOperator *x = Result.Nodes.getNodeAs<clang::BinaryOperator>("sea"))
        {
            LOG_VERBOSE("{sea}: ", util::logging::dump_to_str<const clang::Stmt *>(x, ctx));
            if (const types::Target *t = check_add_seqinjection(x))
            {
                if (const clang::CXXOperatorCallExpr *oop = Result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("oop"))
                {
                    // overloaded [] operator lhs
                }
                else if (const clang::ArraySubscriptExpr *arr =
                             Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("array"))
                {
                    // array subscript
                }
                else
                {
                    // native/trivial sequential assignment
                }
                map_sequential_injections_.at(active_sequent_func_)
                    .push_back(std::move(std::make_shared<BinarySInj>(x, prefix, *t)));
                map_injected_targets_.at(active_sequent_func_).insert({ prefix, t });
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
                    // TODO: function call assigning new values to arg1_expr, revert back to synchronous injection, b.c.
                    // hard to deduce assigned bits. This might need a thorough rework.
                    map_sequential_injections_.at(active_sequent_func_)
                        .push_back(std::move(std::make_shared<CallSInj>(x, arg, prefix, *t)));
                    map_injected_targets_.at(active_sequent_func_).insert({ prefix, t });
                }
            }
        }
    }
}

void InjectionRewriter::BinarySInj::rewrite_injection(const VrtlParser &parser) const
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
    str += prefix_;
    str += get_sequent_injection_stmt(t_, subscripts);

    LOG_INFO("Writing sequential injection point [", str, "] for target: ", t_._self());
    parser.getRewriter().ReplaceText(expr_->getSourceRange(), str);
}

void InjectionRewriter::CallSInj::rewrite_injection(const VrtlParser &parser) const
{
    std::string str = parser.getRewriter().getRewrittenText(expr_->getSourceRange());
    str += "; ";
    str += prefix_;
    str += get_synchronous_injection_stmt(t_);

    parser.getRewriter().ReplaceText(expr_->getSourceRange(), str);
}

void InjectionRewriter::wrap_up_sequent_function(const clang::FunctionDecl *func, const VrtlParser &parser) const
{
    auto &sinjs = map_sequential_injections_.at(func);

    for (const auto &it : sinjs)
    {
        it->rewrite_injection(parser);
    }

    std::stringstream insert;
    insert.str("");
    insert.clear();

    auto &ts = map_injected_targets_.at(func);
    LOG_INFO("INJREW Adding non-dominant injection points to end of sequential function:", func->getNameAsString());

    std::string newc = parser.getRewriter().getRewrittenText(func->getSourceRange());

    LOG_VERBOSE("{compset}: of func ", func->getNameAsString());
    insert << std::endl << "    //>>> VRTLFI non-dominant target injections" << std::endl;

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
    // write injections
    for (const auto &[func, targets] : map_injected_targets_)
    {
        wrap_up_sequent_function(func, parser);
    }
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
