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

#include "Vfiapp_vrtlmod_diffapi.hpp"
#include "Vfiapp_vrtlmodapi.hpp"
#include "Vfiapp.h"
#include "VfiappReference.h"
#include "verilated.h"

#include "fiapp_test.hpp"

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
    VfiappReference original("original");
    VfiappVRTLmodAPIDifferential gDiff(gFault, gRef);

    if (!check_fiapp_target_coverage(gFault))
        return 1;

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

    auto reset = [&](bool background = false) -> void
    {
        gFault.vrtl_.a = gRef.vrtl_.a = 0;
        gFault.vrtl_.enable = gRef.vrtl_.enable = 0;
        gFault.vrtl_.reset = 1;
        gRef.vrtl_.reset = 1;
        clockspin(3);
        gFault.vrtl_.reset = 0;
        gRef.vrtl_.reset = 0;
        if (background)
        {
            gFault.vrtl_.a = gRef.vrtl_.a = 1;
            gFault.vrtl_.enable = gRef.vrtl_.enable = 1;
            clockspin(300);
        }
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
                ret |= 0x1;
            }
        }
        else
        {
            std::cout << "|-> \033[0;31mFailed\033[0m DIFF no difference between faulty and reference RTLs: "
                      << std::endl;
            ret |= 0x2;
        }

        gDiff.diff_target_dictionaries();
        gDiff.dump_diff_csv(std::cout);
        gDiff.dump_diff_csv_vertical(fout);

        // auto triplet_vec_unrolled_masked_calc = gDiff.gen_nz_triplet_vec();
        auto triplet_vec = gDiff.compute_diff_vector();
        gDiff.diff_target_dictionaries(); // recalculate with hard unrolled and masked
        for (auto const &a : triplet_vec)
        {
            std::cout << "X{" << a.target_id_ << "," << a.element_id_ << "," << a.val_ << "}" << std::endl;
            if (gDiff.diff_target(a) == false)
            {
                std::cout << "|-> \033[0;31mFailed\033[0m X DIFF unrolled+masked mismatches rolled+non-masked: {"
                          << a.target_id_ << "," << a.element_id_ << "," << a.val_ << "}" << std::endl;
                ret |= 0x4;
            }
        }
        auto triplet_vec_unrolled_masked_calc = gDiff.gen_nz_triplet_vec();
        gDiff.compute_diff_vector(); // recalculate with hard unrolled and masked
        for (auto const &a : triplet_vec_unrolled_masked_calc)
        {
            std::cout << "Y{" << a.target_id_ << "," << a.element_id_ << "," << a.val_ << "}" << std::endl;
            if (gDiff.diff_target(a) == false)
            {
                std::cout << "|-> \033[0;31mFailed\033[0m Y DIFF unrolled+masked mismatches rolled+non-masked: {"
                          << a.target_id_ << "," << a.element_id_ << "," << a.val_ << "}" << std::endl;
                ret |= 0x8;
            }
        }

        return ret;
    };

    auto equivalence_step = [&](bool clk, bool rst, bool data, bool enable, unsigned event)
    {
        original.clk = gFault.vrtl_.clk = gRef.vrtl_.clk = clk;
        original.reset = gFault.vrtl_.reset = gRef.vrtl_.reset = rst;
        original.a = gFault.vrtl_.a = gRef.vrtl_.a = data;
        original.enable = gFault.vrtl_.enable = gRef.vrtl_.enable = enable;
        original.eval();
        gFault.vrtl_.eval();
        gRef.vrtl_.eval();
        bool equal = original.o1 == gFault.vrtl_.o1 && original.o2 == gFault.vrtl_.o2 &&
                     original.o3 == gFault.vrtl_.o3 && original.state_signature == gFault.vrtl_.state_signature;
        for (unsigned word = 0; word < 3; ++word)
            equal &= original.o4[word] == gFault.vrtl_.o4[word];
        if (!equal)
            std::cerr << "Plain/rewritten fiapp mismatch at event " << event << '\n';
        return equal;
    };
    testreturn &= check_fiapp_equivalence(equivalence_step);

    // VRTL warm-up
    gFault.vrtl_.eval();
    gRef.vrtl_.eval();

    reset();

    std::cout << std::endl << "Running test for simple fault injection application (fiapp)" << "..." << std::endl;
    // test injections

    for (auto &it : gFault.td_)
    {
        if (it.second->injectable_)
            testreturn &= testinject(*(it.second), gFault, clockspin, reset, check_diff,
                                     [&](vrtlfi::td::TDentry &target, unsigned bit)
                                     { return check_fiapp_fault(target, *gRef.td_.at(target.get_name()), bit); });
    }
    if (testreturn && gFault.td_.size() > 0)
    {
        std::cout << std::endl << "..." << "All passed. \033[0;32mTest successful.\033[0m" << std::endl;
    }
    else
    {
        std::cout << std::endl << "..." << "\033[0;31mTest failed.\033[0m" << std::endl;
        return 1;
    }

    return 0;
}
