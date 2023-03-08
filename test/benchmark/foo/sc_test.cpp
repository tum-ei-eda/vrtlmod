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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file sc_test.cpp
/// @date 2023-03-03
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <systemc.h>
#include "verilated.h"

#ifdef VRTLMOD
#include "Vfoo_vrtlmodapi.hpp"
#else
#include "Vfoo.h"
#endif

#include <chrono>

#include <scc/report.h>
#include "scc/router.h"
#include "scc/memory.h"
#include <tlm/scc/tlm_mm.h>
#include <tlm/scc/tlm_id.h>
#include "scc/report.h"

sc_core::sc_signal<bool> rst_n{ "rst" }; // for Active Low RTL core reset
sc_core::sc_signal<bool> o1{ "o0" };
sc_core::sc_signal<bool> o0{ "o1" };

sc_core::sc_clock clk("clk", 10, SC_NS);

// argv[0] this executable
// argv[1] unused!
// argv[2] the scc log level {e.g.: 7=verbose}
// argv[3] the simulation time in miliseconds
int sc_main(int argc, char *argv[])
{
    if (argc < 4)
    {
        return 1;
    }

    sc_report_handler::set_actions(SC_ID_MORE_THAN_ONE_SIGNAL_DRIVER_, SC_DO_NOTHING);
    scc::init_logging(scc::LogConfig()
                          .logLevel(static_cast<scc::log>(std::atoi(argv[2])))
                          .logAsync(false)
                          .dontCreateBroker(false)
                          .coloredOutput(true));
    sc_report_handler::set_actions(SC_ERROR, SC_LOG | SC_CACHE_REPORT | SC_DISPLAY);

    Vfoo *core = nullptr;
#ifdef VRTLMOD
    auto gFrame = std::make_unique<VfooVRTLmodAPI>("vRTLmod");
    core = &(gFrame->vrtl_);

    long long nmb_bits = 0;
    for (const auto &target : gFrame->td_)
    {
        nmb_bits += target.second->bits_;
    }
    std::cout << "> vRTLmod info: a total of " << nmb_bits << " bits in " << gFrame->td_.size()
              << " targetable variables." << std::endl;
#else
    auto gFrame = std::make_unique<Vfoo>("vRTL");
    core = gFrame.get();
#endif

    core->clk_i(clk);
    core->rst_ni(rst_n);
    core->o0_o(o0);
    core->o1_o(o1);
    core->a_i(o1);

    std::clock_t startCPUTime = clock(); // clock() returns CPU time on Linux and wall time on Windows
    auto startWallTime =
        std::chrono::system_clock::now(); // system_clock has wall clock time from the system-wide realtime clock

    rst_n.write(0);
    sc_core::sc_start(10, SC_NS);

    rst_n.write(1);

    sc_core::sc_start(std::atoi(argv[3]), SC_MS);

    std::clock_t endCPUTime = clock();
    auto endWallTime = std::chrono::high_resolution_clock::now();

    double CPUTime = double(endCPUTime - startCPUTime) / double(CLOCKS_PER_SEC);
    std::chrono::duration<double> wallTime = endWallTime - startWallTime;
    auto cycles = sc_core::sc_time_stamp() / sc_core::sc_time{ 10, sc_core::SC_NS };

    double mcps = double(cycles) / CPUTime / 1000000.0;

    std::cout << std::endl;
    std::cout << "************************"
              << "********"
              << "*****" << std::endl;
    std::cout << "*  CPU Simulation Time: " << std::setw(8) << std::setprecision(6) << CPUTime << " s  *" << std::endl;
    std::cout << "* Wall Simulation Time: " << std::setw(8) << std::setprecision(6) << wallTime.count() << " s  *"
              << std::endl;
    std::cout << "*       Simulated Time: " << std::setw(11) << sc_core::sc_time_stamp() << " *" << std::endl;
    std::cout << "*     Simulated Cycles: " << std::setw(8) << std::dec << cycles << "    *" << std::endl;
    std::cout << "*   Performance (MCPS): " << std::setw(8) << std::fixed << std::setprecision(6) << mcps << "    *"
              << std::endl;
    std::cout << "************************"
              << "********"
              << "*****" << std::endl;
    return 0;
}
