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
////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all API building functionalities
namespace apibuild {
////////////////////////////////////////////////////////////////////////////////
/// @class Ext
/// @brief Common base class for extended matchers
/// @author Johannes Geier (johannes.geier@tum.de)
class Ext {
	friend APIbuilder;
protected:
	///////////////////////////////////////////////////////////////////////
	/// \brief Reference to Handler class for access to (re)written text
	ftcv::Handler &_handler;
	///////////////////////////////////////////////////////////////////////
	/// \brief Contains unique identification of main expression
	unsigned mID;
public:
	///////////////////////////////////////////////////////////////////////
	/// \brief Contains start location of main expression
	clang::SourceLocation Begin;
	///////////////////////////////////////////////////////////////////////
	/// \brief Contains end location of main expression
	clang::SourceLocation End;
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns size of main expression calculated from locations
	unsigned size(void) const {
		return (End.getRawEncoding() - Begin.getRawEncoding());
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Checks whether this Ext is in range of given
	/// \param e Pointer to an Ext
	/// \return True if this is in range of e. False if not.
	bool in_range(Ext *e);

	///////////////////////////////////////////////////////////////////////
	/// \brief Contains unique identification of main expression
	unsigned get_ID(void) {
		return (mID);
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns self representation string. Should be specified further by inheriting classes
	virtual std::string _self(void) const {
		return ("Empty Extended Matcher");
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Overloaded operator for stream access of class (reference)
	friend std::ostream& operator<<(std::ostream &os, const Ext &obj) {
		os << obj._self();
		return os;
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Overloaded operator for stream access of class (pointer)
	friend std::ostream& operator<<(std::ostream &os, const Ext *obj) {
		os << *obj;//->_self();
		return os;
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor
	/// \param ID Unique ID (should be initialized by getting unique ID of main expression in inheriting class)
	/// \param handler
	/// \param Begin (from inheriting class)
	/// \param End (from inheriting class)
	Ext(unsigned ID, ftcv::Handler &handler, clang::SourceLocation Begin, clang::SourceLocation End)
			: _handler(handler), mID(ID), Begin(Begin), End(End) {
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Destructor
	virtual ~Ext(void) {
	}

};

////////////////////////////////////////////////////////////////////////////////
/// @class ExtAsgnmnt
/// @brief Extended Assignment matcher class. (pure virtual)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class ExtAsgnmnt: public Ext {
public:
	typedef enum TYPE{
		TRIVIAL, //basically simple data types as base
		ARRAY
	}type_t;
	///////////////////////////////////////////////////////////////////////
	/// \brief Pointer to main expression
	const clang::BinaryOperator *op;
	///////////////////////////////////////////////////////////////////////
	/// \brief Pointer to left hand side of main expression
	const clang::Expr *lhs;
	///////////////////////////////////////////////////////////////////////
	/// \brief Pointer to right hand side of main expression
	const clang::Expr *rhs;
	///////////////////////////////////////////////////////////////////////
	/// \brief String containing the operator
	std::string operatorstr;
	///////////////////////////////////////////////////////////////////////
	/// \brief Instance of this class is most like a (HDL) reset expression
	bool isReset;
private:
	///////////////////////////////////////////////////////////////////////
	/// \brief Type specified for polymorphic accesses
	type_t mType;
public:
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns actual type of specified non abstract inheriting class
	virtual type_t get_type(void) {return (mType);}
	virtual const clang::Expr * get_base(void) = 0;
	virtual const clang::Expr * get_idx(void) = 0;
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns self representation string
	virtual std::string _self(void) const;
	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor
	/// \param op Pointer to main defining expression
	/// \param handler
	/// \param type Specifies and saves type for polymorphic access
	ExtAsgnmnt(const clang::BinaryOperator *op, ftcv::Handler &handler, type_t type);
	///////////////////////////////////////////////////////////////////////
	/// \brief Destructor
	virtual ~ExtAsgnmnt() {
	}
};

////////////////////////////////////////////////////////////////////////////////
/// @class ExtTrivAsgnmnt
/// @brief Trivial assignment:
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class ExtTrivAsgnmnt: public ExtAsgnmnt {
public:
	///////////////////////////////////////////////////////////////////////
	/// \brief Do not use, since trivial assignment has no base
	/// \details Here for specification of pure abstract base class
	const clang::Expr * get_base(void){
		ftcv::log(ftcv::ERROR, "Trivial Assignment has no base");
		return (nullptr);
	};
	///////////////////////////////////////////////////////////////////////
	/// \brief Do not use, since trivial assignment has no index
	/// \details Here for specification of pure abstract base class
	const clang::Expr * get_idx(void){
		ftcv::log(ftcv::ERROR, "Trivial Assignment has no index");
		return (nullptr);
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor
	/// \param op Pointer to main defining expression
	/// \param handler
	ExtTrivAsgnmnt(const clang::BinaryOperator *op, ftcv::Handler &handler)
			: ExtAsgnmnt(op, handler, TYPE::TRIVIAL) {
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Destructor
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
	///////////////////////////////////////////////////////////////////////
	/// \brief Pointer to secondary array expression in array assignment
	const clang::ArraySubscriptExpr *ase;
	///////////////////////////////////////////////////////////////////////
	/// \brief Pointer to array's base
	const clang::Expr *base;
	///////////////////////////////////////////////////////////////////////
	/// \brief Pointer to secondary array's index
	const clang::Expr *idx;
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns array's base
	virtual const clang::Expr * get_base(void){return (base);}
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns array's index
	virtual const clang::Expr * get_idx(void){return (idx);}
	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor
	/// \param ase secondary array expression
	/// \param op Pointer to main defining expression
	/// \param handler
	ExtArrayAsgnmnt(const clang::ArraySubscriptExpr *ase, const clang::BinaryOperator *op, ftcv::Handler &handler)
			: ExtAsgnmnt(op, handler, TYPE::ARRAY), ase(ase), base(ase->getBase()), idx(ase->getIdx()) {

	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Destructor
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
	///////////////////////////////////////////////////////////////////////
	/// \brief Vector containing all assignments in this compound
	std::vector<ExtAsgnmnt*> mAsgmnts;
	///////////////////////////////////////////////////////////////////////
	/// \brief Save an unique assignment found for this compound
	/// \param a To be saved assignment
	/// \return 0 if assignment is already saved. 1 if successfully saved
	int push(ExtAsgnmnt *a);
	///////////////////////////////////////////////////////////////////////
	/// \brief Get all dominant assignments of stored ones
	/// \details A dominant assignment is the control-flow specific last assignment in a compound accessing the same data element
	/// \return Vector containing all dominant assignments
	std::vector<ExtAsgnmnt*> get_dominantAssignments(void);

	///////////////////////////////////////////////////////////////////////
	/// \brief Pointer to main expression
	const clang::Stmt *c;
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns self representation string
	virtual std::string _self(void) const;
	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor
	/// \param c Pointer to main defining expression
	/// \param handler
	ExtCompoundStmt(const clang::Stmt *c, ftcv::Handler &handler)
			: Ext(c->getID(handler.getASTContext()), handler, c->getBeginLoc(), c->getEndLoc()), c(c) {
		Begin = c->getBeginLoc();
		End = c->getEndLoc();
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Destructor
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
	///////////////////////////////////////////////////////////////////////
	/// \brief Vector containing all injected targets in this function
	std::vector<Target*> mInjectedTargets;
	///////////////////////////////////////////////////////////////////////
	/// \brief Vector containing all compounds in this function
	/// \details As specific assignments are sorted by their respective calling compound this is crucial for identifying the actual location
	std::vector<ExtCompoundStmt*> mCompounds;
	///////////////////////////////////////////////////////////////////////
	/// \brief Pointer to main defining expression
	const clang::Decl *f;
	///////////////////////////////////////////////////////////////////////
	/// \brief Returns the declaration string of the function
	std::string get_function_name(void) const;
	///////////////////////////////////////////////////////////////////////
	/// \brief Add a target to the to-inject list
	void addInjTarget(Target &t);
	///////////////////////////////////////////////////////////////////////
	/// \brief Add a compound to this function
	void addCompound(ExtCompoundStmt *x) {
		mCompounds.push_back(x);
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Get the finest-grained compound for a given assignments
	/// \param a Pointer to assignment
	/// \return Compound where assignment is located or nullptr if assignment is not in range of function (not in range of any of its compounds)
	ExtCompoundStmt* return_finest(ExtAsgnmnt *a);

	///////////////////////////////////////////////////////////////////////
	/// \brief Returns self representation string
	virtual std::string _self(void) const;
	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor
	/// \param f Pointer to main defining expression
	/// \param handler
	ExtDeclFunc(const clang::Decl *f, ftcv::Handler &handler)
			: Ext(f->getID(), handler, f->getBeginLoc(), f->getEndLoc()), mCompounds(), f(f) {
	}
	///////////////////////////////////////////////////////////////////////
	/// \brief Destructor
	virtual ~ExtDeclFunc() {
		for (auto &it : mCompounds) {
			delete (it);
		}
	}
};

} // namespace apibuild

#endif /* SRC_APIBUILD_EXTENDEDMATCHERS_HPP_ */
