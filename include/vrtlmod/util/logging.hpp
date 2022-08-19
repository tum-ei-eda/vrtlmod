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
/// @file logging.hpp
/// @modified on Wed Dec 09 13:32:12 2020
////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLMOD_UTIL_LOGGING_HPP__
#define __VRTLMOD_UTIL_LOGGING_HPP__

#include "vrtlmod/util/utility.hpp"

#include "clang/AST/AST.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"

#include <string>

namespace util
{

namespace logging
{

enum LEVEL
{
    OBLIGAT,
    VERBOSE,
    INFO,
    WARNING,
    ERROR
};

const char *toString(LEVEL level);
////////////////////////////////////////////////////////////////////////////////
/// \brief Toggle silent output (false on reset)
void toggle_silent(void);
////////////////////////////////////////////////////////////////////////////////
/// \brief Toggle verbose output (false on reset)
void toggle_verbose(void);
////////////////////////////////////////////////////////////////////////////////
/// \brief Log helper function
void log(LEVEL level, const std::string &msg, bool silent_toggle = false, bool verbose_toggle = false);

template <typename T>
std::string toLogString(const T &obj)
{
    return "{UNKNOWN OBJECT}";
}

template <>
inline std::string toLogString<std::string>(const std::string &obj)
{
    return std::string("{\"") + obj + "\"}";
}

template <>
std::string toLogString<std::tuple<const clang::SourceLocation &, clang::SourceManager &>>(
    const std::tuple<const clang::SourceLocation &, clang::SourceManager &> &cons);

template <>
std::string toLogString<std::tuple<const clang::SourceRange &, clang::SourceManager &>>(
    const std::tuple<const clang::SourceRange &, clang::SourceManager &> &cons);

template <>
inline std::string toLogString<std::tuple<clang::SourceManager &, const clang::SourceRange &>>(
    const std::tuple<clang::SourceManager &, const clang::SourceRange &> &cons);

template <typename T, typename... OT>
void log(LEVEL level, const std::string &msg, const T &o, const OT &...objects)
{
    log(level, msg + "\n\t" + toLogString(o), objects...);
}

template <typename llvm_expr_t>
std::string dump_to_str(llvm_expr_t x, const clang::ASTContext *ctx = nullptr)
{
    std::string ret;
    llvm::raw_string_ostream os(ret);
    if constexpr (std::is_same<llvm_expr_t, const clang::Stmt *>::value)
    {
        x->dump(os, *ctx);
    }
    else
    {
        x->dump(os);
    }
    return ret;
}

template <util::logging::LEVEL level, typename... Strings>
void LOG(Strings &&...strings)
{
    util::logging::log(level, util::concat(std::forward<Strings>(strings)...));
}

} // namespace logging

} // namespace util

template <typename... Strings>
void LOG_VERBOSE(Strings &&...strings)
{
    util::logging::LOG<util::logging::VERBOSE>(std::forward<Strings>(strings)...);
}
template <typename... Strings>
void LOG_INFO(Strings &&...strings)
{
    util::logging::LOG<util::logging::INFO>(std::forward<Strings>(strings)...);
}
template <typename... Strings>
void LOG_ERROR(Strings &&...strings)
{
    util::logging::LOG<util::logging::ERROR>(std::forward<Strings>(strings)...);
}
template <typename... Strings>
void LOG_OBLIGAT(Strings &&...strings)
{
    util::logging::LOG<util::logging::OBLIGAT>(std::forward<Strings>(strings)...);
}
template <typename... Strings>
void LOG_WARNING(Strings &&...strings)
{
    util::logging::LOG<util::logging::WARNING>(std::forward<Strings>(strings)...);
}
template <typename... Strings>
void LOG_FATAL(Strings &&...strings)
{
    util::logging::LOG<util::logging::ERROR>(std::forward<Strings>(strings)...);
    // TODO: break execution here
}
#endif // __VRTLMOD_UTIL_LOGGING_HPP__
