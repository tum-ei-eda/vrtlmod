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
/// @file fiapp_test.cpp
////////////////////////////////////////////////////////////////////////////////

#include "Vfiapp_vrtlmodapi.hpp"
#include "Vfiapp.h"
#include "verilated.h"

#include "testinject.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        return -1;
    }
    std::ofstream fout(argv[1]);

    VfiappVRTLmodAPI gFault;
    VfiappVRTLmodAPI gRef;
    VfiappVRTLmodAPIDifferential gDiff(gFault, gRef);

    bool testreturn = true;

    auto clockspin = [&](int n = 1) -> void
    {
        for (; n > 0; --n)
        {
            gFault.vrtl_.eval();
            gRef.vrtl_.eval();
            gFault.vrtl_.clk = 1;
            gRef.vrtl_.clk = 1;
            gFault.vrtl_.eval();
            gRef.vrtl_.eval();
            gFault.vrtl_.clk = 0;
            gRef.vrtl_.clk = 0;
        }
    };

    auto reset = [&](void) -> void
    {
        gFault.vrtl_.reset = 1;
        gRef.vrtl_.reset = 1;
        clockspin(3);
        gFault.vrtl_.reset = 0;
        gRef.vrtl_.reset = 0;
    };
    auto check_diff = [&](vrtlfi::td::TDentry const *target) -> int
    {
        vrtlfi::td::TDentry const *diff_target = nullptr;
        int ret = 0;
        if (diff_target = gDiff.compare_fast(target); diff_target != nullptr)
        {
            if (diff_target == target)
            {
                // we pass the injection target as argument (search entry) to the compare algorithm. The compare should
                // immediately return the same target.
                std::cout << "|-> \033[0;32mDiff Ok\033[0m DIFF on same target as injected: " << diff_target->get_name()
                          << std::endl;
                ret = 0;
            }
            else
            {
                std::cout << "|-> \033[0;31mFailed\033[0m DIFF on other target {" << diff_target->get_name()
                          << "} than the injected {" << target->get_name() << "}" << std::endl;
                ret = 1;
            }
        }
        else
        {
            std::cout << "|-> \033[0;31mFailed\033[0m DIFF no difference between faulty and reference RTLs: "
                      << std::endl;
            ret = 2;
        }

        gDiff.diff_target_dictionaries();
        gDiff.dump_diff_csv(std::cout);
        gDiff.dump_diff_csv_vertical(fout);

        return ret;
    };

    // VRTL warm-up
    gFault.vrtl_.eval();
    gRef.vrtl_.eval();

    reset();

    std::cout << std::endl
              << "Running test for simple fault injection application (fiapp)"
              << "..." << std::endl;
    // test injections

    for (auto &it : gFault.td_)
    {
        testreturn &= testinject(*(it.second), gFault, clockspin, reset, check_diff);
    }
    if (testreturn && gFault.td_.size() > 0)
    {
        std::cout << std::endl
                  << "..."
                  << "All passed. \033[0;32mTest successful.\033[0m" << std::endl;
    }
    else
    {
        std::cout << std::endl
                  << "..."
                  << "\033[0;31mTest failed.\033[0m" << std::endl;
        return 1;
    }

    return 0;
}
