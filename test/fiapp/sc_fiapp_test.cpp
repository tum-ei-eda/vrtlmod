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
/// @file sc_fiapp_test.cpp
////////////////////////////////////////////////////////////////////////////////

#include "Vfiapp_vrtlmod_diffapi.hpp"
#include "Vfiapp_vrtlmodapi.hpp"
#include "verilated.h"
#include "systemc.h"
#include "Vfiapp.h"
#include "VfiappReference.h"

#include "fiapp_test.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

int sc_main(int argc, char *argv[])
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

    sc_signal<bool> tb_clk{ "clk" };
    sc_signal<bool> tb_reset{ "reset" };
    sc_signal<bool> tb_a{ "a" };
    sc_signal<bool> tb_enable{ "enable" };
    sc_signal<bool> tb_o1{ "o1" }, ref_o1{ "ref_o1" };
    sc_signal<bool> tb_o2{ "o2" }, ref_o2{ "ref_o2" };
    sc_signal<bool> tb_o3{ "o3" }, ref_o3{ "ref_o3" };
    sc_signal<sc_bv<65>> tb_o4{ "o4" }, ref_o4{ "ref_o4" };
    sc_signal<bool> original_o1{ "original_o1" }, original_o2{ "original_o2" }, original_o3{ "original_o3" };
    sc_signal<sc_bv<65>> original_o4{ "original_o4" };
    sc_signal<sc_bv<64>> tb_signature{ "signature" }, ref_signature{ "ref_signature" };
    sc_signal<sc_bv<64>> original_signature{ "original_signature" };

    bool testreturn = true;

    auto clockspin = [&](int n = 1) -> void
    {
        static const sc_core::sc_time halfspintime{ 5.0, sc_core::SC_NS };
        for (; n > 0; --n)
        {
            tb_clk.write(0);
            sc_start(halfspintime);
            tb_clk.write(1);
            sc_start(halfspintime);
        }
    };

    auto reset = [&](bool background = false) -> void
    {
        tb_a.write(0);
        tb_enable.write(0);
        tb_reset.write(1);
        clockspin(3);
        tb_reset.write(0);
        if (background)
        {
            tb_a.write(1);
            tb_enable.write(1);
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

    gFault.vrtl_.clk(tb_clk);
    gFault.vrtl_.reset(tb_reset);
    gFault.vrtl_.a(tb_a);
    gFault.vrtl_.enable(tb_enable);
    gFault.vrtl_.o1(tb_o1);
    gFault.vrtl_.o2(tb_o2);
    gFault.vrtl_.o3(tb_o3);
    gFault.vrtl_.o4(tb_o4);
    gFault.vrtl_.state_signature(tb_signature);

    // Bind the reference core. All Input are equal, output run into dummies
    gRef.vrtl_.clk(tb_clk);
    gRef.vrtl_.reset(tb_reset);
    gRef.vrtl_.a(tb_a);
    gRef.vrtl_.enable(tb_enable);
    gRef.vrtl_.o1(ref_o1);
    gRef.vrtl_.o2(ref_o2);
    gRef.vrtl_.o3(ref_o3);
    gRef.vrtl_.o4(ref_o4);
    gRef.vrtl_.state_signature(ref_signature);

    original.clk(tb_clk);
    original.reset(tb_reset);
    original.a(tb_a);
    original.enable(tb_enable);
    original.o1(original_o1);
    original.o2(original_o2);
    original.o3(original_o3);
    original.o4(original_o4);
    original.state_signature(original_signature);

    auto equivalence_step = [&](bool clk, bool rst, bool data, bool enable, unsigned event)
    {
        tb_clk.write(clk);
        tb_reset.write(rst);
        tb_a.write(data);
        tb_enable.write(enable);
        sc_start(1, SC_NS);
        const bool equal = original_o1.read() == tb_o1.read() && original_o2.read() == tb_o2.read() &&
                           original_o3.read() == tb_o3.read() && original_o4.read() == tb_o4.read() &&
                           original_signature.read() == tb_signature.read();
        if (!equal)
            std::cerr << "Plain/rewritten SystemC fiapp mismatch at event " << event << '\n';
        return equal;
    };
    testreturn &= check_fiapp_equivalence(equivalence_step);

    // VRTL warm-up
    tb_reset.write(0);
    clockspin();

    reset();

    std::cout << std::endl << "Running test for simple fault injection application (fiapp)" << "..." << std::endl;

    // test injections
    for (auto &[kname, vtarget_ptr] : gFault.td_)
    {
        if (vtarget_ptr->injectable_)
        {
            testreturn &= testinject(*vtarget_ptr, gFault, clockspin, reset, check_diff,
                                     [&](vrtlfi::td::TDentry &target, unsigned bit)
                                     { return check_fiapp_fault(target, *gRef.td_.at(target.get_name()), bit); });
        }
    }
    if (testreturn && gFault.td_.size() > 0)
    {
        std::cout << std::endl << "..." << " all passed. \033[0;32mTest successful.\033[0m" << std::endl;
    }
    else
    {
        std::cout << std::endl << "..." << "\033[0;31m test failed.\033[0m" << std::endl;
        return 1;
    }
    return 0;
}
