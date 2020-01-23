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
	bool in_sequent;
	ExtDeclFunc *activeSequentFunc;
	std::vector<ExtDeclFunc*> mSequentFuncs;
	ExtCompoundStmt* activeCompoundStmt;

	void writeIncludeModification();
	void writeSequentInject(ExtDeclFunc* sequent_function);

	void writeSequentInject(const clang::BinaryOperator *op, const clang::Expr *base = nullptr, const clang::Expr *index = nullptr);
	void writeIntermittenInject(void);

	int registerAssignment(ExtAsgnmnt* a);
	void analyzeRewrite(void);
	InjectionRewriter(ftcv::Consumer &cons);
	virtual ~InjectionRewriter(void);
	virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result);

	virtual void addMatcher(clang::ast_matchers::MatchFinder &finder);
private:
	std::set<clang::SourceLocation> visited;
};

} // namespace apibuild

#endif // VREWTL_DOMINANTINJECT_H
