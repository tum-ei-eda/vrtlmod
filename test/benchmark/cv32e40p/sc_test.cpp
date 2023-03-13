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
#include "Vcv32e40p_vrtlmodapi.hpp"
#else
#include "Vcv32e40p.h"
#endif

#include <chrono>

#include <scc/report.h>
#include "scc/router.h"
#include "scc/memory.h"
#include <tlm/scc/tlm_mm.h>
#include <tlm/scc/tlm_id.h>
#include "scc/report.h"

#include "elfio/elfio.hpp"

#include "mif2tlm_bridge.h"
#define INSTR_BRIDGE_TYPE mif2tlm_bridge<32, 32, 4, false, true>
#define DATA_BRIDGE_TYPE mif2tlm_bridge<32, 32, 4, true, false>
#define INSTR_BRIDGE_SIGNALS_TYPE mif_signals<32, 32, 4>
#define DATA_BRIDGE_SIGNALS_TYPE mif_signals<32, 32, 4>

sc_core::sc_signal<bool> rst_n{ "rst" }; // for Active Low RTL core reset

sc_core::sc_clock clk("clk", 10, SC_NS);

#define ADDR_WIDTH__ 32

// argv[0] this executable
// argv[1] the target binary
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

    Vcv32e40p *core = nullptr;
#ifdef VRTLMOD
    auto gFrame = std::make_unique<Vcv32e40pVRTLmodAPI>("vRTLmod");
    core = &(gFrame->vrtl_);

    long long nmb_bits = 0;
    for (const auto &target : gFrame->td_)
    {
        nmb_bits += target.second->bits_;
    }
    std::cout << "> vRTLmod info: a total of " << nmb_bits << " bits in " << gFrame->td_.size()
              << " targetable variables." << std::endl;
#else
    auto gFrame = std::make_unique<Vcv32e40p>("vRTL");
    core = gFrame.get();
#endif
    auto instr_bridge_signals = std::make_shared<INSTR_BRIDGE_SIGNALS_TYPE>("instr_bridge_signals");
    auto data_bridge_signals = std::make_shared<DATA_BRIDGE_SIGNALS_TYPE>("data_bridge_signals");

    core->clk_i(clk);
    core->rst_ni(rst_n);
    core->instr_req_o(instr_bridge_signals->req);
    core->instr_gnt_i(instr_bridge_signals->req); //< automatically grant on request
    core->instr_addr_o(instr_bridge_signals->addr);
    core->instr_rdata_i(instr_bridge_signals->rdata);
    core->instr_rvalid_i(instr_bridge_signals->rvalid);

    core->data_req_o(data_bridge_signals->req);
    core->data_gnt_i(data_bridge_signals->req); //< automatically grant on request
    core->data_rvalid_i(data_bridge_signals->rvalid);
    core->data_rdata_i(data_bridge_signals->rdata);
    core->data_addr_o(data_bridge_signals->addr);
    core->data_we_o(data_bridge_signals->we);
    core->data_be_o(data_bridge_signals->be);
    core->data_wdata_o(data_bridge_signals->wdata);

    auto instr_bridge = std::make_shared<INSTR_BRIDGE_TYPE>("instr_bridge");
    instr_bridge_signals->connect(*instr_bridge);
    instr_bridge->clk(clk);
    instr_bridge->resetn(rst_n);

    auto data_bridge = std::make_shared<DATA_BRIDGE_TYPE>("data_bridge");
    data_bridge_signals->connect(*data_bridge);
    data_bridge->clk(clk);
    data_bridge->resetn(rst_n);

    scc::router<32> router("tlm-bus", 5, 2);
    instr_bridge->socket_.bind((router.target[0]));
    data_bridge->socket_.bind((router.target[1]));

    scc::memory<0x03FFFFFF, 32> plic_dummy("plic");
    router.bind_target(plic_dummy.target, 2, 0x0C000000, 0x03FFFFFF);

    scc::memory<0x000C0000, 32> clint_dummy("clint");
    router.bind_target(clint_dummy.target, 3, 0x02000000, 0x000C0000);

    scc::memory<0x00001000, 32> timer_dummy("timer");
    router.bind_target(timer_dummy.target, 4, 0x18000000, 0x00001000);

    scc::memory<0x00100000, 32> memory("mem");
    router.bind_target(memory.target, 0, 0, 0x00100000);

    scc::memory<0x20, 32> bitbang("uart");
    router.bind_target(bitbang.target, 1, 0x10000000, 0x20);

    bitbang.set_operation_callback(
        [](scc::memory<0x20, 32> &mem, tlm::tlm_generic_payload &gp, sc_core::sc_time &delay) -> int
        {
            auto addr = gp.get_address();
            auto datp = gp.get_data_ptr();
            auto cmd = gp.get_command();
            auto len = gp.get_data_length();
            uint8_t *byt = gp.get_byte_enable_ptr();
            unsigned wid = gp.get_streaming_width();

            SCCTRACE(mem.name()) << (cmd == tlm::TLM_READ_COMMAND ? "read" : "write") << " access to addr 0x"
                                 << std::hex << addr;

            if (addr == 0x0 && cmd == tlm::TLM_WRITE_COMMAND)
            {
                char x[2] = { '\0', '\0' };
                x[0] = datp[0];
                std::cout << x; // << std::flush;
                gp.set_response_status(tlm::TLM_OK_RESPONSE);
            }
            else if (addr == 0x14 && cmd == tlm::TLM_READ_COMMAND)
            {
                datp[0] = 0x60;
                gp.set_response_status(tlm::TLM_OK_RESPONSE);
            }
            else
            {
                gp.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
                return 0;
            }
            gp.set_dmi_allowed(false);
            return len;
        });

    ELFIO::elfio elf_reader;
    elf_reader.load((const char *)(argv[1]));

    std::vector<tlm::tlm_generic_payload *> flashmems;
    for (auto &seg : elf_reader.segments)
    {
        if (seg->get_physical_address() >= 0 && seg->get_physical_address() < (0 + 0x00100000))
        {
            auto seg_data{ seg->get_data() };
            auto trans = prepare_trans(seg->get_file_size());
            auto data = trans->get_data_ptr();
            trans->set_address(seg->get_physical_address());
            trans->set_command(tlm::TLM_WRITE_COMMAND);

            for (size_t i = 0; i < seg->get_file_size(); ++i)
            {
                data[i] = static_cast<uint8_t>(seg_data[i] & 0xff);
            }
            flashmems.push_back(trans);
        }
    }
    for (auto &trans : flashmems)
    {
        trans->acquire();
        router.transport_dbg(0, *trans);
        if (trans->get_response_status() != tlm::TLM_OK_RESPONSE)
            SCCERR() << "Invalid response status" << trans->get_response_string();
        trans->release();
    }

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
