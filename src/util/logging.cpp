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
/// @file logging.cpp
/// @modified on Wed Dec 09 13:32:12 2020 (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "vrtlmod/util/logging.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace util
{

namespace logging
{

const char *toString(LEVEL level)
{
    switch (level)
    {
    case OBLIGAT:
        return "\033[0;36m";
    case VERBOSE:
        return "\033[0;37m>";
    case INFO:
        return "\033[1;37mInfo ";
    case WARNING:
        return "\033[1;33mWarning ";
    case ERROR:
        return "\033[0;31mError ";
    default:
        return "Unknown";
    }
}

void toggle_silent(void)
{
    log(VERBOSE, "", true);
}
void toggle_verbose(void)
{
    log(VERBOSE, "", false, true);
}

void log(LEVEL level, const std::string &msg, bool silent_toggle, bool verbose_toggle)
{
    static bool silent = false;
    static bool verbose = false;
    std::stringstream x;
    if (silent_toggle)
    {
        silent = !silent;
    }
    if (verbose_toggle)
    {
        verbose = !verbose;
    }
    if (level == VERBOSE && !verbose)
    {
        return;
    }

    if ((silent == false) || ((silent == true) && (level == ERROR or level == OBLIGAT)))
    {
        switch (level)
        {
        case ERROR:
            [[fallthrough]];
        case WARNING:
            std::cout << util::concat(toString(level), msg, " \033[0m") << std::endl;
            break;
        default:
            std::cout << util::concat(toString(level), " \033[0m", msg) << std::endl;
            break;
        }
    }
}

template <>
std::string toLogString<std::tuple<const clang::SourceRange &, clang::SourceManager &>>(
    const std::tuple<const clang::SourceRange &, clang::SourceManager &> &cons)
{
    return std::string("{clang::SourceRange {") +
           toLogString(std::tie<const clang::SourceLocation &, clang::SourceManager &>(std::get<0>(cons).getBegin(),
                                                                                       std::get<1>(cons))) +
           "," +
           toLogString(std::tie<const clang::SourceLocation &, clang::SourceManager &>(std::get<0>(cons).getEnd(),
                                                                                       std::get<1>(cons))) +
           "}";
}

template <>
std::string toLogString<std::tuple<const clang::SourceLocation &, clang::SourceManager &>>(
    const std::tuple<const clang::SourceLocation &, clang::SourceManager &> &cons)
{
    std::stringstream ss;
    {
        llvm::raw_os_ostream rout(ss);
        std::get<0>(cons).print(rout, std::get<1>(cons));
        rout.flush();
    }
    return ss.str();
}

template <>
inline std::string toLogString<std::tuple<clang::SourceManager &, const clang::SourceRange &>>(
    const std::tuple<clang::SourceManager &, const clang::SourceRange &> &cons)
{
    return toLogString<std::tuple<const clang::SourceRange &, clang::SourceManager &>>(
        std::tie(std::get<1>(cons), std::get<0>(cons)));
}

} // namespace logging

} // namespace util
