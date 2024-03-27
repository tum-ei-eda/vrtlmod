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
/// @file testinject.cpp
/// @date 2022-23-12
////////////////////////////////////////////////////////////////////////////////

#include <sstream>
#include "testinject.hpp"

bool testinject(vrtlfi::td::TDentry &target, vrtlfi::td::TD_API &api, std::function<void(int)> const &clockspin,
                std::function<void(void)> const &reset,
                std::function<int(vrtlfi::td::TDentry const *)> const &check_diff, std::ostream &out)
{
    bool ret = true;
    for (int i = 0; i < target.bits_; ++i)
    {
        reset();
        out << "\033[1;37mTesting Injection in:\033[0m " << target.get_name() << " bitflip [" << i << "]" << std::endl;
        int cntrsum = 0, cntrsum_new = 0;
        target.reset_mask();
        if (api.prep_inject(target.get_name(), i) != 0)
        {
            out << "|-> \033[0;31mFailed\033[0m - Target not found. Dictionary in corrupt state" << std::endl;
            ret = false;
            continue;
        }
        target.arm();
        std::vector<int> cntr = target.get_cntr();
        auto pre_data = target.read_data();
        clockspin(1);

        auto post_data = target.read_data();
        std::vector<int> cntr_new = target.get_cntr();

        out << " data pre: ";
        for (const auto &bit : pre_data)
        {
            out << bit;
        }
        out << "\ndata post: ";
        for (const auto &bit : post_data)
        {
            out << bit;
        }
        out << "\n";

        std::stringstream x, y;
        out << "CO:\t";
        for (const auto &it : cntr)
        {
            x << "|" << it;
            cntrsum += it;
        }
        out << x.str() << "|" << std::endl << "CN:\t";
        for (const auto &it : cntr_new)
        {
            y << "|" << it;
            cntrsum_new += it;
        }
        out << y.str() << "|" << std::endl;

        if (cntrsum != 0)
        {
            out << "|-> \033[0;31mFailed\033[0m - Target dictionary in corrupt state" << std::endl;
            ret = false;
            continue;
        }
        if (cntrsum_new > 1)
        {
            out << "|-> \033[0;31mFailed\033[0m - More than one injection: " << cntrsum_new << std::endl;
            ret = false;
            continue;
        }
        if (cntrsum_new == 0)
        {
            out << "|-> \033[0;31mFailed\033[0m - No injection" << std::endl;
            ret = false;
            continue;
        }

        if (check_diff(&target) != 0)
        {
            out << "|-> \033[0;31mFailed\033[0m - DIFF problematic." << std::endl;
            ret = false;
            continue;
        }

        out << "|-> \033[0;32mPassed\033[0m" << std::endl;
    }
    return ret;
}
