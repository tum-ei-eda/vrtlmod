#include "Vscheduling_comb_vrtlmodapi.hpp"

#include <array>
#include <iostream>
#include <vector>

int main()
{
    Vscheduling_combVRTLmodAPI api;
    auto &dut = api.vrtl_;
    const std::array<const char *, 3> names{ "TOP.blocking_state", "TOP.clocked_state", "TOP.independent_state" };
    std::array<vrtlfi::td::TDentry *, 3> targets{};
    for (const auto &[name, entry] : api.td_)
    {
        bool found = false;
        for (unsigned index = 0; index < names.size(); ++index)
            if (name == names[index])
            {
                targets[index] = entry;
                found = true;
            }
        if (!found)
        {
            std::cerr << "Unexpected target outside clocked state: " << name << '\n';
            return 1;
        }
    }
    for (unsigned index = 0; index < targets.size(); ++index)
        if (targets[index] == nullptr || !targets[index]->injectable_ || targets[index]->bits_ != 1)
        {
            std::cerr << "Missing one-bit state target: " << names[index] << '\n';
            return 1;
        }

    constexpr unsigned blocking = 1;
    constexpr unsigned clocked = 2;
    constexpr unsigned independent = 4;
    struct Inputs
    {
        bool clk, clk_b, reset, data, data_b, enable;
    };
    struct Event
    {
        const char *name;
        Inputs inputs;
        unsigned disarm;
        unsigned prepare;
        std::array<unsigned, 3> state;
        std::array<int, 3> counters;
    };

    // Inputs: clk, clk_b, reset, data, data_b, enable.
    // State/counters: blocking, derived-clock NBA, independent-clock NBA.
    // These expected values come from the RTL events, independently of read_data
    // and the generated differential API. Preparing a target resets its counter.
    const Event events[] = {
        { "reset", { 0, 0, 1, 0, 0, 1 }, 0, 0, { 0, 0, 0 }, { 0, 0, 0 } },
        { "release reset", { 0, 0, 0, 0, 0, 1 }, 0, 0, { 0, 0, 0 }, { 0, 0, 0 } },
        { "armed input-only evaluation", { 0, 0, 0, 1, 0, 1 }, 0, blocking, { 0, 0, 0 }, { 0, 0, 0 } },
        { "blocking write then flip", { 1, 0, 0, 1, 0, 1 }, 0, 0, { 0, 0, 0 }, { 1, 0, 0 } },

        // Reset asserts while the external clock is held high.
        { "reset-triggered flip", { 1, 0, 1, 1, 0, 1 }, 0, blocking, { 1, 0, 0 }, { 1, 0, 0 } },
        { "release reset and lower clock", { 0, 0, 0, 0, 0, 1 }, blocking, 0, { 1, 0, 0 }, { 1, 0, 0 } },
        { "lower derived clock", { 1, 0, 0, 0, 0, 1 }, 0, 0, { 0, 0, 0 }, { 1, 0, 0 } },
        { "prepare derived-clock state", { 0, 0, 0, 1, 0, 1 }, 0, clocked, { 0, 0, 0 }, { 1, 0, 0 } },
        { "derived-clock write then flip", { 1, 0, 0, 1, 0, 1 }, 0, 0, { 1, 0, 0 }, { 1, 1, 0 } },
        { "repeat evaluation", { 1, 0, 0, 1, 0, 1 }, 0, 0, { 1, 0, 0 }, { 1, 1, 0 } },

        // A disabled register holds a nonzero value. A fault still applies on its
        // clock event, including when no enabled RTL assignment writes the state.
        { "disable and change data", { 0, 0, 0, 0, 0, 0 }, clocked, 0, { 1, 0, 0 }, { 1, 1, 0 } },
        { "disabled nonzero hold", { 1, 0, 0, 0, 0, 0 }, 0, 0, { 1, 0, 0 }, { 1, 1, 0 } },
        { "rearm without clock event", { 1, 0, 0, 0, 0, 0 }, 0, blocking, { 1, 0, 0 }, { 0, 1, 0 } },
        { "armed falling clock edge", { 0, 0, 0, 0, 0, 0 }, 0, 0, { 1, 0, 0 }, { 0, 1, 0 } },
        { "flip held state", { 1, 0, 0, 0, 0, 0 }, 0, 0, { 0, 0, 0 }, { 1, 1, 0 } },
        { "enable with injection disarmed", { 0, 0, 0, 1, 0, 1 }, blocking, 0, { 0, 0, 0 }, { 1, 1, 0 } },
        { "enabled write of one", { 1, 0, 0, 1, 0, 1 }, 0, 0, { 1, 1, 0 }, { 1, 1, 0 } },
        { "disarm before ordinary write", { 0, 0, 0, 0, 0, 1 }, blocking, 0, { 1, 1, 0 }, { 1, 1, 0 } },
        { "ordinary zero write", { 1, 0, 0, 0, 0, 1 }, 0, 0, { 0, 1, 0 }, { 1, 1, 0 } },

        // The independent target is armed with a fresh counter. Activity in the
        // first clock domain must leave both its value and its counter untouched.
        { "arm inactive independent domain", { 1, 0, 0, 0, 1, 1 }, 0, independent, { 0, 1, 0 }, { 1, 1, 0 } },
        { "other domain falling edge", { 0, 0, 0, 0, 1, 1 }, 0, 0, { 0, 1, 0 }, { 1, 1, 0 } },
        { "other domain rising edge", { 1, 0, 0, 0, 1, 1 }, 0, 0, { 0, 1, 0 }, { 1, 1, 0 } },
        { "independent write then flip", { 1, 1, 0, 0, 1, 1 }, 0, 0, { 0, 1, 0 }, { 1, 1, 1 } },
        { "repeat independent evaluation", { 1, 1, 0, 0, 1, 1 }, 0, 0, { 0, 1, 0 }, { 1, 1, 1 } },
        { "disarm independent domain", { 1, 0, 0, 0, 0, 1 }, independent, 0, { 0, 1, 0 }, { 1, 1, 1 } },
        { "independent zero without fault", { 1, 1, 0, 0, 0, 1 }, 0, 0, { 0, 1, 0 }, { 1, 1, 1 } },
        { "change independent data", { 1, 0, 0, 0, 1, 1 }, 0, 0, { 0, 1, 0 }, { 1, 1, 1 } },
        { "independent one without fault", { 1, 1, 0, 0, 1, 1 }, 0, 0, { 0, 1, 1 }, { 1, 1, 1 } },
        { "rearm independent without edge", { 1, 1, 0, 0, 0, 1 }, 0, independent, { 0, 1, 1 }, { 1, 1, 0 } },
        { "other domain after rearm falls", { 0, 1, 0, 0, 0, 1 }, 0, 0, { 0, 1, 1 }, { 1, 1, 0 } },
        { "other domain after rearm rises", { 1, 1, 0, 0, 0, 1 }, 0, 0, { 0, 1, 1 }, { 1, 1, 0 } },
        { "rearmed independent falling edge", { 1, 0, 0, 0, 0, 1 }, 0, 0, { 0, 1, 1 }, { 1, 1, 0 } },
        { "rearmed independent write/flip", { 1, 1, 0, 0, 0, 1 }, 0, 0, { 0, 1, 1 }, { 1, 1, 1 } },

        // Check isolation in the opposite direction, with active-region state
        // armed while only the independent NBA register receives clock events.
        { "arm inactive first domain", { 1, 1, 0, 0, 0, 0 }, independent, blocking, { 0, 1, 1 }, { 0, 1, 1 } },
        { "independent falling edge only", { 1, 0, 0, 0, 0, 0 }, 0, 0, { 0, 1, 1 }, { 0, 1, 1 } },
        { "independent rising edge only", { 1, 1, 0, 0, 0, 0 }, 0, 0, { 0, 1, 0 }, { 0, 1, 1 } },
        { "first domain falling edge", { 0, 1, 0, 0, 0, 0 }, 0, 0, { 0, 1, 0 }, { 0, 1, 1 } },
        { "flip held zero in first domain", { 1, 1, 0, 0, 0, 0 }, 0, 0, { 1, 0, 0 }, { 1, 1, 1 } },
        { "reset with both clocks high", { 1, 1, 1, 0, 0, 1 }, blocking, 0, { 0, 0, 0 }, { 1, 1, 1 } },
        { "release reset without an edge", { 1, 1, 0, 0, 0, 1 }, 0, 0, { 0, 0, 0 }, { 1, 1, 1 } },
    };

    bool passed = true;
    for (const auto &event : events)
    {
        for (unsigned index = 0; index < targets.size(); ++index)
        {
            auto &target = *targets[index];
            if (event.disarm & (1U << index))
                target.disarm();
            if (event.prepare & (1U << index))
            {
                target.reset_mask();
                if (api.prep_inject(target, 0) != 0)
                {
                    std::cerr << event.name << ": failed to prepare " << names[index] << '\n';
                    return 1;
                }
                target.arm();
            }
        }
        dut.clk = event.inputs.clk;
        dut.clk_b = event.inputs.clk_b;
        dut.reset = event.inputs.reset;
        dut.data = event.inputs.data;
        dut.data_b = event.inputs.data_b;
        dut.enable = event.inputs.enable;
        dut.eval();

        const std::array<unsigned, 3> state{ dut.blocking_state, dut.clocked_state, dut.independent_state };
        for (unsigned index = 0; index < targets.size(); ++index)
        {
            if (state[index] != event.state[index])
            {
                std::cerr << event.name << ": " << names[index] << " expected " << event.state[index] << ", got "
                          << state[index] << '\n';
                passed = false;
            }
            if (targets[index]->get_cntr() != std::vector<int>{ event.counters[index] })
            {
                std::cerr << event.name << ": wrong injection counter for " << names[index] << ", expected "
                          << event.counters[index] << '\n';
                passed = false;
            }
        }
        if (dut.combinational != !event.inputs.data)
        {
            std::cerr << event.name << ": combinational output did not follow input\n";
            passed = false;
        }
    }
    return passed ? 0 : 1;
}
