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
/// @file injectionrewriter.cpp
/// @date Created on Mon Jan 09 11:56:10 2020
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/transform/injectionrewriter.hpp"
#include "vrtlmod/transform/extendedmatchers.hpp"
#include "vrtlmod/vapi/generator.hpp"
#include "vrtlmod/vapi/xmlhelper.hpp"

#include <boost/lexical_cast.hpp>

#include "vrtlmod/util/logging.hpp"

#include <sstream>
#include <string>
#include <algorithm>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

namespace transform {

void InjectionRewriter::writeIncludeModification(void) {
	FileID fid = getRewriter().getSourceMgr().getFileID(activeSequentFunc->f->getBeginLoc());
	SourceLocation flocSOF = getRewriter().getSourceMgr().getLocForStartOfFile(fid);
	SourceRange flocra = SourceRange(flocSOF, getRewriter().getSourceMgr().getLocForEndOfFile(fid));

	std::string newc = getRewriter().getRewrittenText(flocra);
	auto includepos = newc.find("#include");
	if (includepos != std::string::npos) {
		std::string x = vapi::VapiGenerator::_i().getInludeStrings();
		if (newc.find(x) == std::string::npos) {
			getRewriter().InsertTextBefore(flocSOF, vapi::VapiGenerator::_i().getTDExternalDecl());
			getRewriter().InsertTextBefore(flocSOF, x);
		}
	}
}

void InjectionRewriter::writeSequentInject(ExtDeclFunc *sequent_function) {
	util::logging::log(util::logging::INFO, (std::string("Writing Injection Points for ") + sequent_function->_self()));
	for (auto &compound : sequent_function->mCompounds) {
		std::vector<ExtAsgnmnt*> tInjectionPoints = compound->get_dominantAssignments();
		util::logging::log(util::logging::INFO, std::string("Number of dominant assignments in compound [") + compound->_self() + std::string("]:") + boost::lexical_cast<std::string>(tInjectionPoints.size()));
		for (auto &a : tInjectionPoints) {
			//if (a->get_type() == ExtAsgnmnt::TRIVIAL) {
				writeSequentInject(a->op);
			//} else //if(assignment->mType == ExtAsgnmnt::ARRAY)
			//{
			//	writeSequentInject(a->op, a->get_base(), a->get_idx());
			//}
		}
	}
}

void InjectionRewriter::writeSequentInject(const BinaryOperator *op, const Expr *base, const Expr *index) { //(const BinaryOperator *op){

	int idx;
	util::logging::log(util::logging::INFO, std::string("op: --- ") + getRewriter().getRewrittenText(op->getSourceRange()).c_str() );
	
	std::string name, s, lhs = getRewriter().getRewrittenText(op->getLHS()->getSourceRange());
	std::vector<std::string> subscripts{};
	
	size_t bropen = lhs.find('[');
	if (bropen != std::string::npos){
		name = lhs.substr(0, bropen);
		size_t lastmajor_open = 0, lastmajor_close = 0;
		size_t pos = 0;
		int brcnt = 0;
		for(char const& c: lhs){
			if(c == '['){
				if(brcnt == 0) {
					lastmajor_open = pos;
				}
				++brcnt;
			}
			if(c == ']'){
				--brcnt;
				if(brcnt == 0) {
					lastmajor_close = pos;
					subscripts.push_back(lhs.substr(lastmajor_open+1, lastmajor_close - (lastmajor_open + 1)));
				}
			}
			++pos;
		}
	} else {
		name = lhs;
	}
	
	idx = vapi::VapiGenerator::_i().isExprTarget(name.c_str());
	
	if (idx >= 0) {
		vapi::Target &t = vapi::VapiGenerator::_i().getExprTarget(idx);
		util::logging::log(util::logging::INFO, std::string("Target found \n\t") + t._self());
		std::string newc = getRewriter().getRewrittenText(op->getSourceRange());
		newc += "; ";
		newc += vapi::VapiGenerator::_i().get_sequentInjectionStmtString(t, subscripts);
		getRewriter().ReplaceText(op->getSourceRange(), newc);
		activeSequentFunc->addInjTarget(t);
		t.mSeqInjCnt++;
	}
}
void InjectionRewriter::writeIntermittenInject(void) {
	std::string newc = getRewriter().getRewrittenText(activeSequentFunc->f->getSourceRange());
	//find last '}' as function body end
	std::stringstream insert;
	insert << std::endl << " \t/* Intermitten (non-dominant target injections) */" << std::endl;
	for (auto const &it : activeSequentFunc->mInjectedTargets) {
		insert << "\t" << vapi::VapiGenerator::_i().get_intermittenInjectionStmtString(*it) << ";" << std::endl;
	}
	insert << "}";

	auto posLastBr = newc.rfind("}");
	if (posLastBr != std::string::npos) {
		newc.erase(posLastBr);
		newc.append(insert.str());
		getRewriter().ReplaceText(activeSequentFunc->f->getSourceRange(), newc);
	}
}

int InjectionRewriter::registerAssignment(ExtAsgnmnt *a) {
	if (a) {
		ExtCompoundStmt *tAssignmentLocation = activeSequentFunc->return_finest(a);
		if (tAssignmentLocation) {
			std::stringstream x;
			x << "New Assignment (Id: " << a->get_ID() << ") " << "in compound (Id: " << tAssignmentLocation->get_ID() << ") " << "of sequent Function (Id: "
					<< activeSequentFunc->get_ID() << ")";
			util::logging::log(util::logging::INFO, x.str());
			int ret = tAssignmentLocation->push(a);
			if (ret <= 0) {
				util::logging::log(util::logging::WARNING, "Caught double registration of Assignment");
				return (0);
			}
		} else {
			util::logging::log(util::logging::ERROR, "Assignment not found in active sequential function");
			return (-1);
		}
	}
	return (1);
}

void InjectionRewriter::wrap_up_active_sequent(void){
	
	writeSequentInject(activeSequentFunc);

	writeIntermittenInject();

	writeIncludeModification();

	in_sequent = false;
}

void InjectionRewriter::run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
	if (Result.Nodes.getNodeAs<Decl>("function")) {
		if (activeSequentFunc != nullptr and in_sequent) {
			util::logging::log(util::logging::INFO, std::string("Matcher LEAVING sequent body of \n\t") + activeSequentFunc->_self());
			wrap_up_active_sequent();
		}
	}

	if (const Decl *f = Result.Nodes.getNodeAs<Decl>("sequent_function")) {
		in_sequent = true;
		activeSequentFunc = new ExtDeclFunc(f, *this);
		mSequentFuncs.push_back(activeSequentFunc);
		util::logging::log(util::logging::INFO, std::string("Matcher ENTER sequent body of: \n\t") + activeSequentFunc->_self());
	}

	if (const Stmt *c = Result.Nodes.getNodeAs<Stmt>("compound")) {
		if (in_sequent) {
			activeCompoundStmt = new ExtCompoundStmt(c, *this);
			activeSequentFunc->addCompound(activeCompoundStmt);
			util::logging::log(util::logging::INFO, std::string("New Compound: ") + activeCompoundStmt->_self());
		}
	}

	if (!in_sequent) {
		return;
	} else {
		if (const BinaryOperator *op = Result.Nodes.getNodeAs<clang::BinaryOperator>("assignment")) {
			if (!inThisFile(op->getBeginLoc()))
				return;
			registerAssignment(new ExtTrivAsgnmnt(op, *this));
		}
		if (const BinaryOperator *op = Result.Nodes.getNodeAs<clang::BinaryOperator>("array_assignment")) {
			if (!inThisFile(op->getBeginLoc()))
				return;
			if (const ArraySubscriptExpr *ase = Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("arraySubscriptExprLHS")) {
				registerAssignment(new ExtArrayAsgnmnt(ase, op, *this));
			}
		}
	}
}

void InjectionRewriter::addMatcher(clang::ast_matchers::MatchFinder &finder) {
	finder.addMatcher(functionDecl().bind("function"), this);
	finder.addMatcher(functionDecl(hasBody(compoundStmt()), matchesName("_sequent_*")).bind("sequent_function"), this);

	finder.addMatcher(binaryOperator(isAssignmentOperator(), hasLHS(arraySubscriptExpr().bind("arraySubscriptExprLHS"))).bind("array_assignment"), this);
	finder.addMatcher(binaryOperator(isAssignmentOperator()).bind("assignment"), this);

	finder.addMatcher(compoundStmt().bind("compound"), this);
}

InjectionRewriter::InjectionRewriter(transform::Consumer &cons) :
		transform::Handler(cons), in_sequent(false), activeSequentFunc(), activeCompoundStmt() {
}

void InjectionRewriter::analyzeRewrite(void) {
	if (mSequentFuncs.size() > 0) {
		std::string filename = std::string(getRewriter().getSourceMgr().getFilename(mSequentFuncs.front()->Begin));
		std::vector<vapi::Target*> setpoints = vapi::VapiGenerator::_i().get_targets();
		std::vector<vapi::Target*> injected;
		auto soll = setpoints.size();
		for (auto &itf : mSequentFuncs) {
			for (auto &it : itf->mInjectedTargets) {
				injected.push_back(it);
				for (unsigned i = 0; i < setpoints.size(); i++) {
					if (it == setpoints.at(i)) {
						setpoints.erase(setpoints.begin() + i);
					}
				}
			}
		}
		std::stringstream x;
		float perc = float(injected.size()) / float(soll) * 100.0;
		x << "Analysis of source" << filename << std::endl;
		x << "Uninjected Targets: " << injected.size() << " of " << soll << " done. (" << perc << " %)" << std::endl;
		x << "Remaining Targets: " << std::endl;
		for (auto &it : setpoints) {
			x << "\t- " << it << std::endl;
		}
		util::logging::log(util::logging::INFO, x.str());
	}
}

void InjectionRewriter::onEndOfTranslationUnit(void) {
	if (activeSequentFunc != nullptr and in_sequent) {
		util::logging::log(util::logging::WARNING, std::string("Matcher ALREADY LEFT sequent body of active\n\t") + activeSequentFunc->_self());
		util::logging::log(util::logging::WARNING, std::string("Invoke wrap-up and rewrite at the end of source file for \n\t") + activeSequentFunc->_self());
		wrap_up_active_sequent();
	}
}

InjectionRewriter::~InjectionRewriter(void) {
	analyzeRewrite();
	for (auto &it : mSequentFuncs) {
		delete (it);
	}
}

} // namespace transform
