////////////////////////////////////////////////////////////////////////////////
/// @file injectionrewriter.hpp
/// @date Created on Mon Jan 09 11:56:10 2020
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_TRANSFORM_INJECTIONREWRITER_HPP__
#define __VRTLMOD_TRANSFORM_INJECTIONREWRITER_HPP__

#include "vrtlmod/transform/consumer.hpp"
#include "vrtlmod/transform/extendedmatchers.hpp"
#include "vrtlmod/vapi/target.hpp"

#include "clang/ASTMatchers/ASTMatchFinder.h"

#include <vector>

////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for LLVM/Clang source to source transformation
namespace transform {

////////////////////////////////////////////////////////////////////////////////
/// @class InjectionRewriter
/// @brief AST Handler/Consumer. Rewrites VRTL source code with extended target injection (sequential and intermitten)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class InjectionRewriter: public transform::Handler {

public:
	///////////////////////////////////////////////////////////////////////
	/// \brief Consumer is currently in a sequential function
	bool in_sequent;
	///////////////////////////////////////////////////////////////////////
	/// \brief Pointer to active sequential function
	ExtDeclFunc *activeSequentFunc;
	///////////////////////////////////////////////////////////////////////
	/// \brief Vector containing pointers to all visited sequential fucntions
	std::vector<ExtDeclFunc*> mSequentFuncs;
	///////////////////////////////////////////////////////////////////////
	/// \brief Pointer to currently active compound statement
	ExtCompoundStmt *activeCompoundStmt;

	///////////////////////////////////////////////////////////////////////
	/// \brief Write include macros for API to active VRTL file
	void writeIncludeModification();

	///////////////////////////////////////////////////////////////////////
	/// \brief Write sequential injection statements by function
	/// \param sequent_function Pointer to extended function reference
	void writeSequentInject(ExtDeclFunc *sequent_function);

	///////////////////////////////////////////////////////////////////////
	/// \brief Prints analysis of rewrite work done
	void analyzeRewrite(void);

	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor
	/// \param cons Reference to consumer handler
	InjectionRewriter(Consumer &cons);
	///////////////////////////////////////////////////////////////////////
	/// \brief Destructor
	virtual ~InjectionRewriter(void);
	///////////////////////////////////////////////////////////////////////
	/// \brief wrap up active sequent matching and start rewriter tools
	void wrap_up_active_sequent(void);
	///////////////////////////////////////////////////////////////////////
	/// \brief ASTmatcher run
	virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result);
	///////////////////////////////////////////////////////////////////////
	/// \brief Adds matchers
	virtual void addMatcher(clang::ast_matchers::MatchFinder &finder);

	virtual void onEndOfTranslationUnit(void);

protected:
	void writeSequentInject(const clang::BinaryOperator *op, const clang::Expr *base = nullptr, const clang::Expr *index = nullptr);
	void writeIntermittenInject(void);

	int registerAssignment(ExtAsgnmnt *a);
private:
	std::set<clang::SourceLocation> visited;
};

} // namespace transform

#endif // __VRTLMOD_TRANSFORM_INJECTIONREWRITER_HPP__
