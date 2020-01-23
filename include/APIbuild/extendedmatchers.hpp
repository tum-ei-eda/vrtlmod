////////////////////////////////////////////////////////////////////////////////
/// @file extendedmatchers.hpp
/// @date Created on Mon Jan 21 15:23:13 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef SRC_APIBUILD_EXTENDEDMATCHERS_HPP_
#define SRC_APIBUILD_EXTENDEDMATCHERS_HPP_

#include "../ftcv/Consumer.h"
#include "../APIbuild/target.hpp"
#include <vector>
#include <iostream>

class APIbuilder;

namespace apibuild {

class Ext {
public:
	friend APIbuilder;

	ftcv::Handler &_handler;

	unsigned mID;
	clang::SourceLocation Begin;
	clang::SourceLocation End;
	unsigned size(void) const {
		return (End.getRawEncoding() - Begin.getRawEncoding());
	}
	bool in_range(Ext *e);

	virtual unsigned ID(void) {
		return (mID);
	}

	virtual std::string _self(void) const {
		return ("Empty Extended Matcher");
	}

	friend std::ostream& operator<<(std::ostream &os, const Ext &obj) {
		os << obj._self();
		return os;
	}
	friend std::ostream& operator<<(std::ostream &os, const Ext *obj) {
		os << obj->_self();
		return os;
	}
	Ext(unsigned ID, ftcv::Handler &handler, clang::SourceLocation Begin, clang::SourceLocation End)
			: _handler(handler), mID(ID), Begin(Begin), End(End) {
	}
	virtual ~Ext(void) {
	}

};

class ExtCompoundStmt;

////////////////////////////////////////////////////////////////////////////////
/// @class ExtAsgnmnt
/// @brief Special clang::Decl
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class ExtAsgnmnt: public Ext {
public:
	typedef enum TYPE{
		TRIVIAL, //basically simple data types as base
		ARRAY
	}type_t;

	const clang::BinaryOperator *op;
	const clang::Expr *lhs;
	const clang::Expr *rhs;
	std::string operatorstr;

	bool isReset;
private:
	type_t mType;
public:
	virtual type_t get_type(void) {return (mType);}
	virtual const clang::Expr * get_base(void) = 0;
	virtual const clang::Expr * get_idx(void) = 0;

	virtual std::string _self(void) const;

	ExtAsgnmnt(const clang::BinaryOperator *op, ftcv::Handler &handler, type_t type);

	virtual ~ExtAsgnmnt() {
	}
};

////////////////////////////////////////////////////////////////////////////////
/// @class ExtTrivAsgnmnt
/// @brief Special clang::Decl
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class ExtTrivAsgnmnt: public ExtAsgnmnt {
public:
	ExtTrivAsgnmnt(const clang::BinaryOperator *op, ftcv::Handler &handler)
			: ExtAsgnmnt(op, handler, TYPE::TRIVIAL) {

	}

	const clang::Expr * get_base(void){
		ftcv::log(ftcv::ERROR, "Trivial Assignment has no base");
		return (nullptr);
	};
	const clang::Expr * get_idx(void){
		ftcv::log(ftcv::ERROR, "Trivial Assignment has no index");
		return (nullptr);
	}

	virtual ~ExtTrivAsgnmnt() {
	}
};

////////////////////////////////////////////////////////////////////////////////
/// @class ExtArrayAsgnmnt
/// @brief Special clang::Decl
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class ExtArrayAsgnmnt: public ExtAsgnmnt {
public:
	const clang::ArraySubscriptExpr *ase;
	const clang::Expr *base;
	const clang::Expr *idx;

	virtual const clang::Expr * get_base(void){return (base);}
	virtual const clang::Expr * get_idx(void){return (idx);}

	ExtArrayAsgnmnt(const clang::ArraySubscriptExpr *ase, const clang::BinaryOperator *op, ftcv::Handler &handler)
			: ExtAsgnmnt(op, handler, TYPE::ARRAY), ase(ase), base(ase->getBase()), idx(ase->getIdx()) {

	}

	virtual ~ExtArrayAsgnmnt() {
	}
};

////////////////////////////////////////////////////////////////////////////////
/// @class ExtCompoundStmt
/// @brief Special clang::Decl to save already sequentially injected targets in function
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class ExtCompoundStmt: public Ext {
public:
	std::vector<ExtAsgnmnt*> mAsgmnts;

	int push(ExtAsgnmnt *a);
	std::vector<ExtAsgnmnt*> get_dominantAssignments(void);

	const clang::Stmt *c;
	virtual std::string _self(void) const;

	ExtCompoundStmt(const clang::Stmt *c, ftcv::Handler &handler)
			: Ext(c->getID(handler.getASTContext()), handler, c->getBeginLoc(), c->getEndLoc()), c(c) {
		Begin = c->getBeginLoc();
		End = c->getEndLoc();
	}
	virtual ~ExtCompoundStmt() {
		for (auto &it : mAsgmnts) {
			delete (it);
		}
	}
};

////////////////////////////////////////////////////////////////////////////////
/// @class ExtDeclFunc
/// @brief Special clang::Decl to save already sequentially injected targets in function
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class ExtDeclFunc: public Ext {
public:
	std::vector<Target*> mInjectedTargets;
	std::vector<ExtCompoundStmt*> mCompounds;

	const clang::Decl *f;
	std::string get_function_name(void) const;
	void addInjTarget(Target &t);

	void addCompound(ExtCompoundStmt *x) {
		mCompounds.push_back(x);
	}

	ExtCompoundStmt* return_finest(ExtAsgnmnt *a);

	unsigned ID(void) {
		return (f->getID());
	}
	virtual std::string _self(void) const;

	ExtDeclFunc(const clang::Decl *f, ftcv::Handler &handler)
			: Ext(f->getID(), handler, f->getBeginLoc(), f->getEndLoc()), mCompounds(), f(f) {
	}
	virtual ~ExtDeclFunc() {
		for (auto &it : mCompounds) {
			delete (it);
		}
	}
};

} // namespace apibuild

#endif /* SRC_APIBUILD_EXTENDEDMATCHERS_HPP_ */
