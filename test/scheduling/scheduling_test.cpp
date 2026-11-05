#include "Vscheduling_comb_vrtlmodapi.hpp"

#include <iostream>

int main()
{
    Vscheduling_combVRTLmodAPI api;
    auto &dut = api.vrtl_;
    vrtlfi::td::TDentry *blocking = nullptr;
    vrtlfi::td::TDentry *clocked = nullptr;
    for (const auto &[name, entry] : api.td_)
    {
        if (name == "TOP.blocking_state")
            blocking = entry;
        else if (name == "TOP.clocked_state")
            clocked = entry;
        else
        {
            std::cerr << "Unexpected target outside clocked state: " << name << '\n';
            return 1;
        }
    }
    if (blocking == nullptr || clocked == nullptr || !blocking->injectable_ || !clocked->injectable_)
    {
        std::cerr << "Missing blocking or nonblocking state target\n";
        return 1;
    }

    bool passed = true;
    auto check = [&](bool condition, const char *message) {
        if (!condition)
        {
            std::cerr << message << '\n';
            passed = false;
        }
    };
    auto arm = [&](vrtlfi::td::TDentry &target) {
        target.reset_mask();
        target.reset_cntr();
        check(api.prep_inject(target, 0) == 0, "Failed to prepare injection");
        target.arm();
    };
    auto once = [](vrtlfi::td::TDentry &target) { return target.get_cntr() == std::vector<int>{ 1 }; };

    dut.clk = 0;
    dut.reset = 1;
    dut.data = 0;
    dut.eval();
    dut.reset = 0;
    dut.eval();
    check(dut.blocking_state == 0 && dut.clocked_state == 0, "Asynchronous reset failed");

    arm(*blocking);
    dut.data = 1;
    dut.eval();
    check(dut.combinational == 0, "Combinational output did not follow input");
    check(dut.blocking_state == 0 && blocking->get_cntr() == std::vector<int>{ 0 },
          "Input-only evaluation injected into clocked state");
    dut.clk = 1;
    dut.eval();
    check(dut.blocking_state == 0 && once(*blocking), "Blocking state was not flipped exactly once");
    blocking->disarm();

    // Assert reset while the external clock is held high: no clock edge is needed.
    arm(*blocking);
    dut.reset = 1;
    dut.eval();
    check(dut.blocking_state == 1 && once(*blocking), "Reset-triggered assignment was not injected once");
    blocking->disarm();
    dut.reset = 0;
    dut.clk = 0;
    dut.data = 0;
    dut.eval();
    dut.clk = 1;
    dut.eval();
    check(dut.blocking_state == 0, "Failed to lower derived clock");

    arm(*clocked);
    dut.clk = 0;
    dut.data = 1;
    dut.eval();
    dut.clk = 1;
    dut.eval();
    check(dut.blocking_state == 1 && dut.clocked_state == 0 && once(*clocked),
          "Nonblocking state was not flipped exactly once on the derived clock");
    dut.eval();
    check(dut.clocked_state == 0 && once(*clocked), "Repeated evaluation reinjected without a clock edge");
    return passed ? 0 : 1;
}
