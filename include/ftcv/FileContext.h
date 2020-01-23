////////////////////////////////////////////////////////////////////////////////
/// @file FileContext.h
////////////////////////////////////////////////////////////////////////////////

#ifndef FTCV_FILECONTEXT_H
#define FTCV_FILECONTEXT_H

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include "clang/AST/AST.h"
#include "clang/Rewrite/Core/Rewriter.h"


namespace ftcv
{

/** \brief Handles status of Files and supports clang rewriter.
  */
class FileContext
{
public:
    FileContext(clang::Rewriter & rw,const std::string & file);
    ~FileContext();

    inline void signalChange()
    {
        changed_ = true;
        anyChange_ = true;
    }
    inline bool hasChange()
    {
        return changed_;
    }

    inline void resetIncompatibleChangeFlag()
    {
        incompatibleChange_ = false;
    }
    inline bool hasIncompatibleChange()
    {
        return incompatibleChange_;
    }
    inline void signalIncompatibleChange()
    {
        incompatibleChange_ = true;
        anyChange_ = true;
    }
    inline void signalFatalFailure()
    {
        fatalFailure_ = true;
    }
public:

    clang::Rewriter & rewriter_;
    const std::string file_;
    clang::ASTContext * context_;

private:

    bool changed_;
    bool incompatibleChange_;
    static bool fatalFailure_;
    static bool anyChange_;
public:
    static void resetAnyChangeFlag();
    static bool anyChange();
    static bool fatalFailure();
};

} // namespace ftcv

#endif // FTCV_FILECONTEXT_H
