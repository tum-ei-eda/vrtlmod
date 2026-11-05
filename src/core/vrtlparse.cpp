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
/// @file vrtlparse.cpp
/// @date Created on Tue Feb 08 11:00:31 2022
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/core/vrtlparse.hpp"
#include "vrtlmod/passes/pass.hpp"

#include "vrtlmod/util/logging.hpp"

#include <sstream>
#include <string>
#include <algorithm>
#include <functional>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

namespace vrtlmod
{

void VrtlParser::run(const clang::ast_matchers::MatchFinder::MatchResult &Result)
{
    auto ctx = Result.Context;

    if (const clang::CompoundStmt *x = Result.Nodes.getNodeAs<clang::CompoundStmt>("compound_of_sequent_func"))
    {
        last_seq_compound_begin_ = x->getBeginLoc();
        last_seq_compound_end_ = x->getEndLoc();
        last_seq_compound_ctx_ = ctx;
        const auto *function = Result.Nodes.getNodeAs<clang::FunctionDecl>("sequent_function_def");
        const auto function_name = function ? function->getNameAsString() : std::string{};
        last_seq_is_comb_ = function_name.find("___act_comb__") != std::string::npos
                            || function_name.find("___nba_comb__") != std::string::npos;
        LOG_VERBOSE("{comp}: ", get_source_code_str(x));
    }

    for (const auto &pass : passes_)
    {
        pass->action(*this, Result);
    }
}

bool VrtlParser::is_delayed_commit(const clang::ast_matchers::MatchFinder::MatchResult &result) const
{
    std::function<bool(const clang::Stmt *)> reads_delayed = [&](const clang::Stmt *stmt) {
        if (!stmt)
            return false;
        const clang::NamedDecl *decl = nullptr;
        if (const auto *member = llvm::dyn_cast<clang::MemberExpr>(stmt))
            decl = member->getMemberDecl();
        else if (const auto *reference = llvm::dyn_cast<clang::DeclRefExpr>(stmt))
            decl = reference->getDecl();
        if (decl && decl->getNameAsString().find("__Vdly") == 0)
            return true;
        for (const auto *child : stmt->children())
            if (reads_delayed(child))
                return true;
        return false;
    };

    for (const auto &[id, node] : result.Nodes.getMap())
        if (id.find("sea_binary_") == 0)
            if (const auto *assignment = node.get<clang::BinaryOperator>())
                return reads_delayed(assignment->getRHS());

    if (const auto *call = result.Nodes.getNodeAs<clang::CallExpr>("sea_func"))
        if (const auto *callee = call->getDirectCallee())
            for (const auto &[name, destination] : func_macros_regex_paraidx_)
                if (name == callee->getNameAsString())
                    for (unsigned index = 0; index < call->getNumArgs(); ++index)
                        if (index != static_cast<unsigned>(destination) && reads_delayed(call->getArg(index)))
                            return true;
    return false;
}

void VrtlParser::addMatcher(clang::ast_matchers::MatchFinder &finder)
{
    // clang-format off
    const auto vrtl_internal = anyOf(
        matchesName("::__V"),
        matchesName("__VinpClk__"),
        matchesName("__Vdly__"),
        matchesName("__Vm_traceActivity__"),
        matchesName("__Vclklast__"),
        matchesName("__Vtable__"),
        matchesName("__Vcellinp__"),
        matchesName("__Vchglast__"),
        matchesName("__Vcellout__"),
        matchesName("__Vlvbound__"),
        matchesName("__Vtableidx__"),
        matchesName("__Vfunc__"),
        matchesName("__Vilp__"),
        matchesName("__Vdpiimwrap__"),
        matchesName("__Vconst__"),
        matchesName("__Vdpi_"),
        matchesName("::vlNamep$")
    );

    const auto sc_module_decl =
        cxxRecordDecl(isDirectlyDerivedFrom("sc_core::sc_module"))
            .bind("module"); ///<  decl type of systemc modules (can match systemc verilated modules)

    const auto cc_module_decl = cxxRecordDecl(
                                    anyOf(isDirectlyDerivedFrom("VerilatedModule"),
                                          allOf(has(fieldDecl(hasName("vlSymsp"),
                                                              hasType(pointsTo(cxxRecordDecl(matchesName("__Syms$")))))),
                                                unless(isDerivedFrom("VerilatedModel")))))
                                    .bind("module"); ///<  decl type of cc modules (can match cc verilated modules that
                                                     ///<  inherit from Verilator sim class)

    const auto symtable_decl =
        cxxRecordDecl(isDirectlyDerivedFrom("VerilatedSyms"))
            .bind(
                "module"); ///<  decl type that can match the single symbol table that comes with each verilated system

    const auto topref_decl =
        fieldDecl(allOf(isPublic(), hasAncestor(symtable_decl),
#if VRTLMOD_VERILATOR_VERSION <= 4204
                        anyOf(hasType(pointsTo(sc_module_decl)), hasType(pointsTo(cc_module_decl)),
                              hasType(references(sc_module_decl)), hasType(references(cc_module_decl))),
                        matchesName("::TOPp")
#else // VRTLMOD_VERILATOR_VERSION <= 4.228
                        anyOf(hasType(sc_module_decl), hasType(cc_module_decl)),
                        hasName("TOP")
#endif
                            ),
                  unless(vrtl_internal))
            .bind("topref_decl"); ///< field declarations referencing the top module

    const auto cell_decl =
        fieldDecl(allOf(isPublic(), anyOf(hasAncestor(sc_module_decl), hasAncestor(cc_module_decl)),
                        anyOf(hasType(pointsTo(sc_module_decl)), hasType(pointsTo(cc_module_decl)),
                              hasType(references(sc_module_decl)), hasType(references(cc_module_decl))),
#if VRTLMOD_VERILATOR_VERSION <= 4204
                        unless(matchesName("::TOPp")
#else // VRTLMOD_VERILATOR_VERSION <= 4.228
                        unless(matchesName("::TOP")
#endif
                                   )),
                  unless(vrtl_internal))
            .bind("cell_decl"); ///< field declarations referencing a cell of a verilated module

    const auto instance_decl =
        fieldDecl(
            allOf(isPublic(), hasAncestor(symtable_decl), anyOf(hasType(sc_module_decl), hasType(cc_module_decl))),
            unless(vrtl_internal))
            .bind("instance_decl"); ///< field declarations of verilated modules (non reference) = instances

    const auto signal_decl =
        fieldDecl(
            isPublic(), unless(isImplicit()),
            anyOf(hasAncestor(sc_module_decl), hasAncestor(cc_module_decl)), unless(vrtl_internal),
#if VRTLMOD_VERILATOR_VERSION <= 4204
            unless(hasType(pointsTo(
                cxxRecordDecl()))) // we can not use {sc,cc}_module_decl fine grained matching because verilator builds
                                   // its dependant modules with forward declarations not resolvable from header AST
#else // VERILATOR_VERSION <= 4.228
            unless(anyOf(hasType(pointsTo(cxxRecordDecl())),
                         hasType(referenceType()),
                         hasType(namedDecl(matchesName("string")))
                     ))
#endif
            )
            .bind(
                "signal_decl"); ///< field declarations of verilated signals (non reference) = instances of data storage

    const auto compound_of_sequent_func =
        compoundStmt(hasParent(functionDecl(
#if VRTLMOD_VERILATOR_VERSION <= 4204
            anyOf(
                matchesName("::_sequent__*"),
                matchesName("::_multiclk__*")
            )
#elif VRTLMOD_VERILATOR_VERSION < 5000
            anyOf(
                matchesName("__sequent__*"),
                matchesName("__multiclk__*")
            )
#else
            // Verilator 5 schedules clocked logic in the active and NBA regions.
            // Input-combinational (ico) and settle (stl) functions also contain
            // "sequent" in their names, but are not clocked injection locations.
            anyOf(
                matchesName("___(act|nba)_sequent__"),
                matchesName("___(act|nba)_comb__"),
                // Recent releases inline small region functions into these drivers.
                matchesName("___eval_(act|nba)(__[0-9]+)?$"),
                matchesName("___eval_body__(act|nba)(__[0-9]+)?$")
            )
#endif
        ).bind("sequent_function_def")))
            .bind("compound_of_sequent_func"); ///< compound statements (`{...}`) of sequential evaluation functions

    const auto symtable_expr = expr(hasType(pointsTo(cxxRecordDecl(matchesName("__Syms$")))))
                                  .bind("symboltable");
    const auto symtable_object_expr = expr(anyOf(
        symtable_expr,
        memberExpr(hasDeclaration(fieldDecl(isImplicit())), hasObjectExpression(symtable_expr))));
    const auto symtable_storage_expr = expr(anyOf(
        symtable_expr,
        memberExpr(hasDeclaration(fieldDecl(hasName("vlInst"))), hasObjectExpression(symtable_object_expr))));
    const auto symtbl_instance_expr =
                memberExpr(                   // {instance}
                    hasObjectExpression(symtable_storage_expr),
                    anyOf(hasType(pointsTo(sc_module_decl)), hasType(pointsTo(cc_module_decl)), hasType(sc_module_decl),
                          hasType(cc_module_decl)))
                    .bind("instance"); // instance member of symboltable
    const auto symtbl_instance_object_expr = expr(anyOf(
        symtbl_instance_expr,
        memberExpr(hasDeclaration(fieldDecl(isImplicit())), hasObjectExpression(symtbl_instance_expr))));

    const auto symtbl_instance_signal_expr = // {symboltable}->{instance}.{signal}
        memberExpr(                          // {signal}
            hasObjectExpression(anyOf(
                symtbl_instance_expr,
                memberExpr(hasDeclaration(fieldDecl(hasName("__VprimaryPorts"))),
                           hasObjectExpression(symtbl_instance_object_expr)).bind("signal_base")
                )),
            unless(anyOf(hasDeclaration(namedDecl(matchesName("::__V"))),
                         hasDeclaration(namedDecl(matchesName("->__V"))),
                         hasDeclaration(namedDecl(matchesName(".__V"))),
                         hasType(namedDecl(matchesName("string")))
                     )))
            .bind("signal"); ///< signal member of instance reference

    const auto symtbl_instance_wsignal_expr = // {symboltable}->{instance}.{signal}implicit{wsignal}
        memberExpr(                           // {wsignal_field}, e.g. VlWide int*
            hasObjectExpression(symtbl_instance_signal_expr))
            .bind("wsignal_field"); ///< signal member of instance reference;

    const auto module_object_expr = expr(anyOf(
                                implicitCastExpr(anyOf(hasImplicitDestinationType(pointsTo(sc_module_decl)),
                                                       hasImplicitDestinationType(pointsTo(cc_module_decl)),
                                                       hasImplicitDestinationType(references(cc_module_decl)),
                                                       hasImplicitDestinationType(references(cc_module_decl)))),
                                // Verilator 5 can access the module through vlSelfRef.
                                declRefExpr(anyOf(hasType(sc_module_decl), hasType(cc_module_decl)))
                                )).bind("top");
    const auto module_storage_expr = expr(anyOf(
        module_object_expr,
        memberExpr(hasDeclaration(fieldDecl(isImplicit())), hasObjectExpression(module_object_expr))));

    const auto top_signal_expr = // vlTOPp->{signal}
        memberExpr(              // {signal}
            hasObjectExpression(anyOf(
                module_object_expr,
                memberExpr(hasDeclaration(fieldDecl(hasName("__VprimaryPorts"))),
                           hasObjectExpression(module_storage_expr)).bind("top")
                )),
            unless(anyOf(hasDeclaration(namedDecl(matchesName("::__V"))),
                         hasDeclaration(namedDecl(matchesName("->__V"))),
                         hasDeclaration(namedDecl(matchesName(".__V"))),
                         hasType(namedDecl(matchesName("string")))
                         )))
            .bind("signal"); ///< signal member of instance reference

    const auto top_wsignal_expr = // vlTOPp->{signal}implcit{wsignal}
        memberExpr(               // {wsignal_field}, e.g. VlWide int*
            hasObjectExpression(top_signal_expr))
            .bind("wsignal_field");

    const auto this_signal_expr = // this->{signal}
        memberExpr(               // {signal}
            hasObjectExpression(
                cxxThisExpr(anyOf(hasType(pointsTo(sc_module_decl)), hasType(pointsTo(cc_module_decl)))).bind("this")),
            unless(anyOf(hasDeclaration(namedDecl(matchesName("::__V"))),
                         hasDeclaration(namedDecl(matchesName("->__V"))),
                         hasDeclaration(namedDecl(matchesName(".__V"))),
                         hasType(namedDecl(matchesName("string")))
             )))
            .bind("signal"); ///< signal member of instance reference

    const auto this_wsignal_expr = // vlTOPp->{signal}implcit{wsignal}
        memberExpr(                // {wsignal_field}, e.g. VlWide int*
            hasObjectExpression(this_signal_expr))
            .bind("wsignal_field");

    const auto seq_assignment_matcher = binaryOperator(
        isAssignmentOperator(),
        hasLHS(anyOf(
            top_wsignal_expr,
            top_signal_expr,
            symtbl_instance_wsignal_expr,
            symtbl_instance_signal_expr,
            this_wsignal_expr,
            this_signal_expr,
            cxxOperatorCallExpr(
                hasOverloadedOperatorName("[]"),
                hasArgument(
                    0, anyOf(
                        //>>>
                        hasDescendant(top_wsignal_expr),
                        hasDescendant(top_signal_expr),
                        hasDescendant(symtbl_instance_wsignal_expr),
                        hasDescendant(symtbl_instance_signal_expr),
                        hasDescendant(this_wsignal_expr),
                        hasDescendant(this_signal_expr) //<<< this block of expr solves multi-dimensional x[][][]
                        ,

                        //>>>
                        top_wsignal_expr,
                        top_signal_expr,
                        symtbl_instance_wsignal_expr,
                        symtbl_instance_signal_expr,
                        this_wsignal_expr,
                        this_signal_expr //<<< this block of expr solves onedimensional x[]
                        )
                    )
                )
                .bind("oop")

            ,
            arraySubscriptExpr(
                hasBase(
                anyOf(
                    //>>> this is odd it gives the {signal} match to the subscript not the base
                    hasDescendant(top_wsignal_expr),
                    hasDescendant(top_signal_expr),
                    hasDescendant(symtbl_instance_wsignal_expr),
                    hasDescendant(symtbl_instance_signal_expr),
                    hasDescendant(this_wsignal_expr),
                    hasDescendant(this_signal_expr) //<<< this block of expr solves multi-dimensional x[][][]
                    ,
                    //>>>
                    top_wsignal_expr,
                    top_signal_expr,
                    symtbl_instance_wsignal_expr,
                    symtbl_instance_signal_expr,
                    this_wsignal_expr,
                    this_signal_expr //<<< this block of expr solves onedimensional x[]
                ))
                )
                .bind("array")

            )
        )
        //,hasAncestor(functionDecl(hasBody(compoundStmt()), matchesName("::_sequent_*")))
        // we do not match like this easily because Ancestor and Parent only matches direct upward-relatives
        // check with is_in_sequent() lambda in traversal
    ); ///< trivial sequential assignment (`a=...`, `a[..]=...`, etc.)

    const auto sea_binary_trivial = binaryOperator(
        isAssignmentOperator(),
        hasLHS(
            anyOf(
                top_wsignal_expr,
                top_signal_expr,
                symtbl_instance_wsignal_expr,
                symtbl_instance_signal_expr,
                this_wsignal_expr,
                this_signal_expr
            )
        )
    );

    const auto sea_binary_array_1d = binaryOperator(
        isAssignmentOperator(),
        hasLHS(
            arraySubscriptExpr(
                hasBase(
                    anyOf(
                        implicitCastExpr(hasSourceExpression(top_wsignal_expr)),
                        implicitCastExpr(hasSourceExpression(top_signal_expr)),
                        implicitCastExpr(hasSourceExpression(symtbl_instance_wsignal_expr)),
                        implicitCastExpr(hasSourceExpression(symtbl_instance_signal_expr)),
                        implicitCastExpr(hasSourceExpression(this_wsignal_expr)),
                        implicitCastExpr(hasSourceExpression(this_signal_expr)) //<<< this block of expr solves onedimensional x[]
                        ,
                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(top_wsignal_expr)))),
                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(top_signal_expr)))),
                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(symtbl_instance_wsignal_expr)))),
                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(symtbl_instance_signal_expr)))),
                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(this_wsignal_expr)))),
                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(this_signal_expr)))) //<<< this block of expr solves onedimensional x[]
                        //,
                        //implicitCastExpr(hasSourceExpression(hasDescendant(top_wsignal_expr))),
                        //implicitCastExpr(hasSourceExpression(hasDescendant(top_signal_expr))),
                        //implicitCastExpr(hasSourceExpression(hasDescendant(symtbl_instance_wsignal_expr))),
                        //implicitCastExpr(hasSourceExpression(hasDescendant(symtbl_instance_signal_expr))),
                        //implicitCastExpr(hasSourceExpression(hasDescendant(this_wsignal_expr))),
                        //implicitCastExpr(hasSourceExpression(hasDescendant(this_signal_expr))) //<<< this block of expr solves onedimensional x[]
                        //,
                        //implicitCastExpr(hasDescendant(top_wsignal_expr)),
                        //implicitCastExpr(hasDescendant(top_signal_expr)),
                        //implicitCastExpr(hasDescendant(symtbl_instance_wsignal_expr)),
                        //implicitCastExpr(hasDescendant(symtbl_instance_signal_expr)),
                        //implicitCastExpr(hasDescendant(this_wsignal_expr)),
                        //implicitCastExpr(hasDescendant(this_signal_expr)) //<<< this block of expr solves onedimensional x[]
                    )
                )
            ).bind("array_1d")
        )
        ,
        hasParent(compoundStmt())
    );

    const auto sea_binary_array_2d = binaryOperator(
        isAssignmentOperator(),
        hasLHS(
            arraySubscriptExpr(
                hasBase(
                    implicitCastExpr(
                        hasSourceExpression(
                            arraySubscriptExpr(
                                hasBase(
                                    anyOf(
                                        implicitCastExpr(hasSourceExpression(top_wsignal_expr)),
                                        implicitCastExpr(hasSourceExpression(top_signal_expr)),
                                        implicitCastExpr(hasSourceExpression(symtbl_instance_wsignal_expr)),
                                        implicitCastExpr(hasSourceExpression(symtbl_instance_signal_expr)),
                                        implicitCastExpr(hasSourceExpression(this_wsignal_expr)),
                                        implicitCastExpr(hasSourceExpression(this_signal_expr)) //<<< this block of expr solves onedimensional x[]
                                        ,
                                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(top_wsignal_expr)))),
                                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(top_signal_expr)))),
                                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(symtbl_instance_wsignal_expr)))),
                                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(symtbl_instance_signal_expr)))),
                                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(this_wsignal_expr)))),
                                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(this_signal_expr)))) //<<< this block of expr solves onedimensional x[]
                                        //,
                                        //implicitCastExpr(hasSourceExpression(hasDescendant(top_wsignal_expr))),
                                        //implicitCastExpr(hasSourceExpression(hasDescendant(top_signal_expr))),
                                        //implicitCastExpr(hasSourceExpression(hasDescendant(symtbl_instance_wsignal_expr))),
                                        //implicitCastExpr(hasSourceExpression(hasDescendant(symtbl_instance_signal_expr))),
                                        //implicitCastExpr(hasSourceExpression(hasDescendant(this_wsignal_expr))),
                                        //implicitCastExpr(hasSourceExpression(hasDescendant(this_signal_expr))) //<<< this block of expr solves onedimensional x[]
                                    )
                                )
                            ).bind("array_1d")
                        )
                    )
                )
            ).bind("array_2d")
        )
    );

    const auto sea_binary_array_oop_oop_3d = binaryOperator(
        isAssignmentOperator(),
        hasLHS(
            arraySubscriptExpr(
                hasBase(
                    implicitCastExpr(
                        hasSourceExpression(
                            cxxMemberCallExpr(
                                has(
                                    memberExpr(
                                        has(
                                            cxxOperatorCallExpr(
                                                hasOverloadedOperatorName("[]"),
                                                hasArgument(
                                                    0,
                                                    cxxOperatorCallExpr(
                                                        hasOverloadedOperatorName("[]"),
                                                        hasArgument(
                                                            0,
                                                            anyOf(
                                                                //>>>
                                                                top_wsignal_expr,
                                                                top_signal_expr,
                                                                symtbl_instance_wsignal_expr,
                                                                symtbl_instance_signal_expr,
                                                                this_wsignal_expr,
                                                                this_signal_expr //<<< this block of expr solves onedimensional x[]
                                                            )
                                                        )
                                                    ).bind("oop_1d")
                                                )
                                            ).bind("oop_2d")
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            ).bind("array_3d")
        )
    );

    const auto sea_binary_array_oop_2d = binaryOperator(
        isAssignmentOperator(),
        hasLHS(
            arraySubscriptExpr(
                hasBase(
                    implicitCastExpr(
                        hasSourceExpression(
                            cxxMemberCallExpr(
                                has(
                                    memberExpr(
                                        has(
                                            cxxOperatorCallExpr(
                                                hasOverloadedOperatorName("[]"),
                                                hasArgument(
                                                    0,
                                                    anyOf(
                                                        //>>>
                                                        top_wsignal_expr,
                                                        top_signal_expr,
                                                        symtbl_instance_wsignal_expr,
                                                        symtbl_instance_signal_expr,
                                                        this_wsignal_expr,
                                                        this_signal_expr //<<< this block of expr solves onedimensional x[]
                                                    )
                                                )
                                            ).bind("oop_1d")
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            ).bind("array_2d")
        )
    );

    const auto sea_binary_array_3d = binaryOperator(
        isAssignmentOperator(),
        hasLHS(
            arraySubscriptExpr(
                hasBase(
                    implicitCastExpr(
                        hasSourceExpression(
                            arraySubscriptExpr(
                                hasBase(
                                    implicitCastExpr(
                                        hasSourceExpression(
                                            arraySubscriptExpr(
                                                hasBase(
                                                    anyOf(
                                                        implicitCastExpr(hasSourceExpression(top_wsignal_expr)),
                                                        implicitCastExpr(hasSourceExpression(top_signal_expr)),
                                                        implicitCastExpr(hasSourceExpression(symtbl_instance_wsignal_expr)),
                                                        implicitCastExpr(hasSourceExpression(symtbl_instance_signal_expr)),
                                                        implicitCastExpr(hasSourceExpression(this_wsignal_expr)),
                                                        implicitCastExpr(hasSourceExpression(this_signal_expr)) //<<< this block of expr solves onedimensional x[]
                                                        ,
                                                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(top_wsignal_expr)))),
                                                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(top_signal_expr)))),
                                                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(symtbl_instance_wsignal_expr)))),
                                                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(symtbl_instance_signal_expr)))),
                                                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(this_wsignal_expr)))),
                                                        implicitCastExpr(hasSourceExpression(cxxMemberCallExpr(has(this_signal_expr)))) //<<< this block of expr solves onedimensional x[]
                                                        //,
                                                        //implicitCastExpr(hasSourceExpression(hasDescendant(top_wsignal_expr))),
                                                        //implicitCastExpr(hasSourceExpression(hasDescendant(top_signal_expr))),
                                                        //implicitCastExpr(hasSourceExpression(hasDescendant(symtbl_instance_wsignal_expr))),
                                                        //implicitCastExpr(hasSourceExpression(hasDescendant(symtbl_instance_signal_expr))),
                                                        //implicitCastExpr(hasSourceExpression(hasDescendant(this_wsignal_expr))),
                                                        //implicitCastExpr(hasSourceExpression(hasDescendant(this_signal_expr))) //<<< this block of expr solves onedimensional x[]
                                                    )
                                                )
                                            ).bind("array_1d")
                                        )
                                    )
                                )
                            ).bind("array_2d")
                        )
                    )
                )
            ).bind("array_3d")
        )
    );

    const auto sea_binary_oop_1d = binaryOperator(
        isAssignmentOperator(),
        hasLHS(
            cxxOperatorCallExpr(
                hasOverloadedOperatorName("[]"),
                hasArgument(
                    0,
                    anyOf(
                        //>>>
                        top_wsignal_expr,
                        top_signal_expr,
                        symtbl_instance_wsignal_expr,
                        symtbl_instance_signal_expr,
                        this_wsignal_expr,
                        this_signal_expr //<<< this block of expr solves onedimensional x[]
                    )
                )
            ).bind("oop_1d")
        )
    );

    const auto sea_binary_oop_2d = binaryOperator(
        isAssignmentOperator(),
        hasLHS(
            cxxOperatorCallExpr(
                hasOverloadedOperatorName("[]"),
                hasArgument(
                    0,
                    cxxOperatorCallExpr(
                        hasOverloadedOperatorName("[]"),
                        hasArgument(
                            0,
                            anyOf(
                                //>>>
                                top_wsignal_expr,
                                top_signal_expr,
                                symtbl_instance_wsignal_expr,
                                symtbl_instance_signal_expr,
                                this_wsignal_expr,
                                this_signal_expr //<<< this block of expr solves onedimensional x[]
                            )
                        )
                    ).bind("oop_1d")
                )
            ).bind("oop_2d")
        )
    );

    const auto sea_binary_oop_3d = binaryOperator(
        isAssignmentOperator(),
        hasLHS(
            cxxOperatorCallExpr(
                hasOverloadedOperatorName("[]"),
                hasArgument(
                    0,
                    cxxOperatorCallExpr(
                        hasOverloadedOperatorName("[]"),
                        hasArgument(
                            0,
                            cxxOperatorCallExpr(
                                hasOverloadedOperatorName("[]"),
                                hasArgument(
                                    0,
                                    anyOf(
                                        //>>>
                                        top_wsignal_expr,
                                        top_signal_expr,
                                        symtbl_instance_wsignal_expr,
                                        symtbl_instance_signal_expr,
                                        this_wsignal_expr,
                                        this_signal_expr //<<< this block of expr solves onedimensional x[]
                                    )
                                )
                            ).bind("oop_1d")
                        )
                    ).bind("oop_2d")
                )
            ).bind("oop_3d")
        )
    );

    finder.addMatcher(compoundStmt().bind("compound"), this);

    finder.addMatcher(topref_decl, this);
    finder.addMatcher(instance_decl, this);
    finder.addMatcher(cell_decl, this);
    finder.addMatcher(signal_decl, this);
    finder.addMatcher(compound_of_sequent_func, this);
    finder.addMatcher(functionDecl().bind("function"), this);

    // matches any non complex member assignment.
    finder.addMatcher(sea_binary_trivial.bind("sea_binary_trivial"), this);

    // matches: VlWide<>
    finder.addMatcher(sea_binary_array_1d.bind("sea_binary_array_1d"), this);
    // matches: WData[][] (should not be the used after VERILATOR version>4.202)
    finder.addMatcher(sea_binary_array_2d.bind("sea_binary_array_2d"), this);
    // matches: WData[][][] (should not be the used after VERILATOR version>4.202)
    finder.addMatcher(sea_binary_array_2d.bind("sea_binary_array_3d"), this);

    // matches: VlUnpacked<>
    finder.addMatcher(sea_binary_oop_1d.bind("sea_binary_oop_1d"), this);
    // matches: VlUnpacked<VlUnpacked<>>
    finder.addMatcher(sea_binary_oop_2d.bind("sea_binary_oop_2d"), this);
    // matches: VlUnpacked<VlUnpacked<VlUnpacked<>>>
    finder.addMatcher(sea_binary_oop_3d.bind("sea_binary_oop_3d"), this);

    // matches: VlUnpacked<VlWide<>>
    finder.addMatcher(sea_binary_array_oop_2d.bind("sea_binary_array_oop_2d"), this);
    // matches: VlUnpacked<VlUnpacked<VlWide<>>>
    finder.addMatcher(sea_binary_array_oop_oop_3d.bind("sea_binary_array_oop_oop_3d"), this);

    const auto wide_destination_signal = expr(anyOf(
        top_wsignal_expr, top_signal_expr,
        symtbl_instance_wsignal_expr, symtbl_instance_signal_expr,
        this_wsignal_expr, this_signal_expr));
    // Follow only array bases; signals used as indices remain readable inputs.
    const auto wide_destination_1d = cxxOperatorCallExpr(
        hasOverloadedOperatorName("[]"), hasArgument(0, wide_destination_signal));
    const auto wide_destination_2d = cxxOperatorCallExpr(
        hasOverloadedOperatorName("[]"), hasArgument(0, wide_destination_1d));
    const auto wide_destination_3d = cxxOperatorCallExpr(
        hasOverloadedOperatorName("[]"), hasArgument(0, wide_destination_2d));
    const auto wide_destination = expr(anyOf(
        wide_destination_signal, wide_destination_1d,
        wide_destination_2d, wide_destination_3d)).bind("arg1_expr");

    for (const auto &it : func_macros_regex_paraidx_)
    {
        const auto seq_assignment_func_matcher = callExpr(
            allOf(
                callee(
                    functionDecl(
                        matchesName("(^|::)" + it.first + "$")
                    )
                ),
                hasArgument(
                    it.second,
                    anyOf(
                        // Newer Verilator wraps writable wide arguments in a
                        // WDataOutP constructor instead of a conversion operator.
                        ignoringImpCasts(cxxConstructExpr(
                            hasType(cxxRecordDecl(hasName("WDataOutP"))),
                            hasArgument(0, wide_destination))),
                        cxxMemberCallExpr(
                            anyOf(
                                callee(top_wsignal_expr),
                                callee(top_signal_expr),
                                callee(symtbl_instance_wsignal_expr),
                                callee(symtbl_instance_signal_expr),
                                callee(this_signal_expr),
                                callee(this_wsignal_expr)
                            ) // callees
                        ).bind("arg1_expr"),
                        expr(top_wsignal_expr).bind("arg1_expr"),
                        expr(top_signal_expr).bind("arg1_expr"),
                        expr(symtbl_instance_wsignal_expr).bind("arg1_expr"),
                        expr(symtbl_instance_signal_expr).bind("arg1_expr"),
                        expr(this_signal_expr).bind("arg1_expr"),
                        expr(this_wsignal_expr).bind("arg1_expr")
                        ,
                        cxxMemberCallExpr(
                            has(
                                memberExpr(
                                    has(
                                        cxxOperatorCallExpr(
                                            hasOverloadedOperatorName("[]"),
                                            hasArgument(
                                                0,
                                                anyOf(
                                                    //>>>
                                                    top_wsignal_expr,
                                                    top_signal_expr,
                                                    symtbl_instance_wsignal_expr,
                                                    symtbl_instance_signal_expr,
                                                    this_wsignal_expr,
                                                    this_signal_expr //<<< this block of expr solves onedimensional x[]
                                                )
                                            )
                                        ).bind("oop_1d")
                                    )
                                )
                            )
                        ).bind("arg1_expr")
                        ,
                        cxxMemberCallExpr(
                            has(
                                memberExpr(
                                    has(
                                        cxxOperatorCallExpr(
                                            hasOverloadedOperatorName("[]"),
                                            hasArgument(
                                                0,
                                                cxxOperatorCallExpr(
                                                    hasOverloadedOperatorName("[]"),
                                                    hasArgument(
                                                        0,
                                                        anyOf(
                                                            //>>>
                                                            top_wsignal_expr,
                                                            top_signal_expr,
                                                            symtbl_instance_wsignal_expr,
                                                            symtbl_instance_signal_expr,
                                                            this_wsignal_expr,
                                                            this_signal_expr //<<< this block of expr solves onedimensional x[]
                                                        )
                                                    )
                                                ).bind("oop_1d")
                                            )
                                        ).bind("oop_2d")
                                    )
                                )
                            )
                        ).bind("arg1_expr")
                    )
                )
            )
        );
        finder.addMatcher(seq_assignment_func_matcher.bind("sea_func"), this);
    }
    // clang-format on
}

void VrtlParser::onEndOfTranslationUnit(void)
{
    for (const auto &pass : passes_)
    {
        pass->end_of_translation(*this);
    }
}

void VrtlParser::add_pass(std::unique_ptr<VrtlmodPass> pass)
{
    passes_.insert(std::move(pass));
}

VrtlParser::VrtlParser(Consumer &cons) : Handler(cons) {}

} // namespace vrtlmod
