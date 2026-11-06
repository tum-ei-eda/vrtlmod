// SPDX-License-Identifier: Apache-2.0
#include "Vcounter_vrtlmodapi.hpp"

#include <iostream>

int main()
{
    VcounterVRTLmodAPI simulation;
    auto &model = simulation.vrtl_;
    const auto tick = [&]()
    {
        model.clk = 0;
        model.eval();
        model.clk = 1;
        model.eval();
        model.clk = 0;
        model.eval();
    };

    model.clk = 0;
    model.reset = 1;
    model.enable = 1;
    model.eval();
    tick();
    model.reset = 0;
    tick();
    if (model.count != 1)
    {
        std::cerr << "Counter initialization failed\n";
        return 1;
    }

    vrtlfi::td::TDentry *target = nullptr;
    for (const auto &entry : simulation.td_)
    {
        std::cout << entry.first << ": " << entry.second->bits_ << " bits, injectable=" << entry.second->injectable_
                  << '\n';
        if (entry.second->injectable_ && entry.second->bits_ == 8)
        {
            if (target != nullptr)
            {
                std::cerr << "Expected exactly one injectable 8-bit counter\n";
                return 1;
            }
            target = entry.second;
        }
    }
    if (target == nullptr)
    {
        std::cerr << "No injectable counter found\n";
        return 1;
    }

    target->disarm();
    target->reset_mask();
    if (simulation.prep_inject(*target, 0) != vrtlfi::td::TD_API::GENERIC_OK)
    {
        std::cerr << "Cannot prepare the bit flip\n";
        return 1;
    }
    target->arm();
    tick();
    target->disarm();

    // The next ordinary count is 2. Flipping bit 0 changes it to 3.
    const unsigned actual = model.count;
    std::cout << "Injected bit 0 of " << target->get_name() << ": count=" << actual << " (expected 3)\n";
    if (actual != (2U ^ 1U))
        return 1;
    model.final();
    return 0;
}
