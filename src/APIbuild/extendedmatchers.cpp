////////////////////////////////////////////////////////////////////////////////
/// @file extendedmatchers.cpp
/// @date Created on Mon Jan 21 15:23:13 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "APIbuild/extendedmatchers.hpp"
#include <string>
#include <sstream>
#include <iostream>

namespace apibuild {

bool Ext::in_range(Ext* e){
	if ((Begin.getRawEncoding() >= e->Begin.getRawEncoding()) and (End.getRawEncoding() <= e->End.getRawEncoding())){
		return true;
	}
	return false;
}


std::string ExtDeclFunc::get_function_name(void) const {
	std::string x = _handler.getRewriter().getRewrittenText(f->getSourceRange());
	x = x.substr(0, x.find("{") - 1);
	return (x);
}

void ExtDeclFunc::addInjTarget(Target &t) {
	bool isInList = false;
	for (auto const &it : mInjectedTargets) {
		if (it == &t) {
			isInList = true;
			break;
		}
	}
	if (!isInList)
		mInjectedTargets.push_back(&t);
}

std::string ExtAsgnmnt::_self(void) const {
	std::stringstream ret;
	ret << "(Id: " << mID << ")" << std::endl << _handler.getRewriter().getRewrittenText(op->getSourceRange());
	return (ret.str());
}

ExtAsgnmnt::ExtAsgnmnt(const clang::BinaryOperator *op, ftcv::Handler &handler, type_t type)
		: Ext(op->getID(handler.getASTContext()), handler, op->getBeginLoc(), op->getEndLoc()), op(op), lhs(op->getLHS()), lhsStr(handler.getRewriter().getRewrittenText(op->getLHS()->getSourceRange())), rhs(op->getRHS()), operatorstr(handler.getRewriter().getRewrittenText(op->getOperatorLoc())), isReset(false), mType(type) {
	std::string rhss = _handler.getRewriter().getRewrittenText(rhs->getSourceRange());
	if (rhss == "0U") {
		isReset = true;
	}
}

int ExtCompoundStmt::push(ExtAsgnmnt *a) {
	for (auto const & it: mAsgmnts){
		if (it->op == a->op){
			return (0);
		}
	}
	mAsgmnts.push_back(a);
	return (1);
}

std::string ExtCompoundStmt::_self(void) const {
	std::stringstream ret;
	ret << "(Id: "<< c->getID(_handler.getASTContext()) << ")";
	return (ret.str());
}

std::vector<ExtAsgnmnt*> ExtCompoundStmt::get_dominantAssignments(void){

	std::vector<ExtAsgnmnt*> x;
//	for (auto i = mAsgmnts.size(); i>0; i--){
	for (auto const & outer: mAsgmnts){
//		ExtAsgnmnt* outer = mAsgmnts[i-1];
		bool found = false;

//		std::string outerLHS = _handler.getRewriter().getRewrittenText(outer->lhs->getSourceRange());
		for (auto & inner: x) {
//			std::string innerLHS = _handler.getRewriter().getRewrittenText(inner->lhs->getSourceRange());
			if ((outer->lhsStr == inner->lhsStr) and (inner->Begin < outer->Begin)){
//			if ((outer->lhsStr == inner->lhsStr) and (outer->Begin < inner->Begin)){
				inner = outer;
				found = true;
				break;
			}
		}
		if(!found){
			x.push_back(outer);
		}
	}
	return (x);
}

std::string ExtDeclFunc::_self(void) const {
	std::stringstream ret;
	ret << "(Id: " << mID << ")" << get_function_name();
	return (ret.str());
}

ExtCompoundStmt* ExtDeclFunc::return_finest(ExtAsgnmnt* a){
	std::vector<ExtCompoundStmt*> inRanges;
	if(!(a->in_range(this))){
		return (nullptr);
	}
	for (auto & it: mCompounds){
		if(a->in_range(it)){
			inRanges.push_back(it);
		}
	}
	for(unsigned i = 0; i< inRanges.size()-1; i++){
		auto min_pos = i;
		for(auto j = i +1; j < inRanges.size(); j++){
			if(inRanges[j]->size() < inRanges[min_pos]->size()){
				min_pos = j;
			}
		}
		ExtCompoundStmt* temp = inRanges[i];
		inRanges[i] = inRanges[min_pos];
		inRanges[min_pos] = temp;
	}
	return (inRanges.front());
}

} // namespace apibuild
