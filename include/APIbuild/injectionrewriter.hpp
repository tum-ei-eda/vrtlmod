////////////////////////////////////////////////////////////////////////////////
/// @file injectionrewriter.hpp
/// @date Created on Mon Jan 09 11:56:10 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#ifndef VREWTL_DOMINANTINJECT_H
#define VREWTL_DOMINANTINJECT_H

#include "../APIbuild/target.hpp"
#include "../ftcv/Consumer.h"
#include "../APIbuild/extendedmatchers.hpp"
#include <vector>

class InjectionRewriter;

namespace apibuild {
////////////////////////////////////////////////////////////////////////////////
/// @class InjectionRewriter
/// @brief AST Handler/Consumer. Rewrites VRTL source code with extended target injection (sequential and intermitten)
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////
class InjectionRewriter: public ftcv::Handler {

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
	ExtCompoundStmt* activeCompoundStmt;

	///////////////////////////////////////////////////////////////////////
	/// \brief Write include macros for API to active VRTL file
	void writeIncludeModification();

	///////////////////////////////////////////////////////////////////////
	/// \brief Write sequential injection statements by function
	/// \param sequent_function Pointer to extended function reference
	void writeSequentInject(ExtDeclFunc* sequent_function);

	///////////////////////////////////////////////////////////////////////
	/// \brief Prints analysis of rewrite work done
	void analyzeRewrite(void);

	///////////////////////////////////////////////////////////////////////
	/// \brief Constructor
	/// \param cons Reference to consumer handler
	InjectionRewriter(ftcv::Consumer &cons);
	///////////////////////////////////////////////////////////////////////
	/// \brief Destructor
	virtual ~InjectionRewriter(void);
	///////////////////////////////////////////////////////////////////////
	/// \brief ASTmatcher run
	virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result);
	///////////////////////////////////////////////////////////////////////
	/// \brief Adds matchers
	virtual void addMatcher(clang::ast_matchers::MatchFinder &finder);

protected:
	void writeSequentInject(const clang::BinaryOperator *op, const clang::Expr *base = nullptr, const clang::Expr *index = nullptr);
	void writeIntermittenInject(void);

	int registerAssignment(ExtAsgnmnt* a);
private:
	std::set<clang::SourceLocation> visited;
};

} // namespace apibuild

#endif // VREWTL_DOMINANTINJECT_H
