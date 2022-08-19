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
/// @file test.cpp
/// @date Created on Mon Jan 25 19:29:05 2020
/// @author Johannes Geier (johannes.geier@tum.de)
////////////////////////////////////////////////////////////////////////////////

#include "Vfiapp_vrtlmodapi.hpp"
#include "verilated.h"
#include "systemc.h"
#include "Vfiapp.h"

#include <iostream>
#include <sstream>

VfiappVRTLmodAPI &gFrame = VfiappVRTLmodAPI::i();
#define gVtop (*(gFrame.vrtl_))

sc_signal<bool> tb_clk{ "clk" };
sc_signal<bool> tb_reset{ "reset" };
sc_signal<bool> tb_a{ "a" };
sc_signal<bool> tb_enable{ "enable" };
sc_signal<bool> tb_o1{ "o1" };
sc_signal<bool> tb_o2{ "o2" };
sc_signal<bool> tb_o3{ "o3" };
sc_signal<sc_bv<65>> tb_o4{ "o4" };

void clockspin(void)
{
    static const sc_core::sc_time halfspintime{ 5.0, sc_core::SC_NS };
    tb_clk.write(0);
    sc_start(halfspintime);
    tb_clk.write(1);
    sc_start(halfspintime);
}

bool testinject(vrtlfi::td::TDentry &target, vrtlfi::td::TD_API &api)
{
    bool ret = true;
    for (int i = 0; i < target.bits_; ++i)
    {
        std::cout << "\033[1;37mTesting Injection in:\033[0m " << target.get_name() << " bitflip [" << i << "]"
                  << std::endl;
        int cntrsum = 0, cntrsum_new = 0;
        target.reset_mask();
        if (api.prep_inject(target.get_name(), i) != 0)
        {
            std::cout << "|-> \033[0;31mFailed\033[0m - Target not found. Dictionary in corrupt state" << std::endl;
            ret = false;
            continue;
        }
        target.arm();
        std::vector<int> cntr = target.get_cntr();
        auto pre_data = target.read_data();
        clockspin();
        auto post_data = target.read_data();
        std::vector<int> cntr_new = target.get_cntr();

        std::cout << " data pre: ";
        for (const auto &bit : pre_data)
        {
            std::cout << bit;
        }
        std::cout << "\ndata post: ";
        for (const auto &bit : post_data)
        {
            std::cout << bit;
        }
        std::cout << "\n";

        std::stringstream x, y;
        std::cout << "CO:\t";
        for (const auto &it : cntr)
        {
            x << "|" << it;
            cntrsum += it;
        }
        std::cout << x.str() << "|" << std::endl << "CN:\t";
        for (const auto &it : cntr_new)
        {
            y << "|" << it;
            cntrsum_new += it;
        }
        std::cout << y.str() << "|" << std::endl;

        if (cntrsum != 0)
        {
            std::cout << "|-> \033[0;31mFailed\033[0m - Target dictionary in corrupt state" << std::endl;
            ret = false;
            continue;
        }
        if (cntrsum_new > 1)
        {
            std::cout << "|-> \033[0;31mFailed\033[0m - More than one injection: " << cntrsum_new << std::endl;
            ret = false;
            continue;
        }
        if (cntrsum_new == 0)
        {
            std::cout << "|-> \033[0;31mFailed\033[0m - No injection" << std::endl;
            ret = false;
            continue;
        }
        std::cout << "|-> \033[0;32mPassed\033[0m" << std::endl;
    }
    return ret;
}

int sc_main(int argc, char *argv[])
{

    gVtop.clk(tb_clk);
    gVtop.reset(tb_reset);
    gVtop.a(tb_a);
    gVtop.enable(tb_enable);
    gVtop.o1(tb_o1);
    gVtop.o2(tb_o2);
    gVtop.o3(tb_o3);
    gVtop.o4(tb_o4);

    // VRTL warum-up
    tb_reset.write(0);
    clockspin();
    std::cout << std::endl
              << "Running test for simple fault injection application (fiapp)"
              << "..." << std::endl;
    // test injections
    bool testreturn = true;
    for (auto &it : gFrame.td_)
    {
        testreturn &= testinject(*(it.second), gFrame);
    }
    if (testreturn && gFrame.td_.size() > 0)
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
        return (1);
    }
    return (0);
}
