#include <Vlayout__Syms.h>
#include <Vlayout_vrtlmodapi.hpp>

#include <array>
#include <iostream>
#include <vector>

double sc_time_stamp() { return 0; }

int main()
{
    VlayoutVRTLmodAPI api;
    auto& root = *api.vrtl_.rootp;
    const std::array<const char*, 4> names{ "TOP.state", "TOP.nested", "TOP.port", "TOP.flag" };
    const std::array<unsigned, 4> bits{ 32, 1, 65, 1 };
    const std::array<unsigned, 4> flip{ 0, 0, 64, 0 };
    if (api.td_.size() != names.size() || &root != &root.vlSymsp->vlInst.TOP)
    {
        std::cerr << "Wrong named-layout inventory or root alias\n";
        return 1;
    }
    for (unsigned index = 0; index < names.size(); ++index)
    {
        auto found = api.td_.find(names[index]);
        if (found == api.td_.end() || !found->second->injectable_ || found->second->bits_ != bits[index])
        {
            std::cerr << "Missing injectable target " << names[index] << '\n';
            return 1;
        }
        auto& target = *found->second;
        target.reset_mask();
        if (api.prep_inject(target, flip[index]) != 0)
            return 1;
        target.arm();
        api.vrtl_.eval();
        // Inspect the actual model storage rather than the API's read_data.
        if (root.state != (index == 0 ? 4U : 5U) || root.nested != (index == 1)
            || root.__VprimaryPorts.port[0] != 0 || root.__VprimaryPorts.port[1] != 0
            || root.__VprimaryPorts.port[2] != (index == 2) || root.__VprimaryPorts.flag != (index == 3))
        {
            std::cerr << "Injection reached the wrong storage for " << names[index] << '\n';
            return 1;
        }
        const auto counters = target.get_cntr();
        const auto counter_index = flip[index] / 32;
        if (counters.size() != (bits[index] + 31) / 32)
            return 1;
        for (unsigned word = 0; word < counters.size(); ++word)
            if (counters[word] != (word == counter_index ? 1 : 0))
            {
                std::cerr << "Wrong injection counter for " << names[index] << '\n';
                return 1;
            }
        target.disarm();
        api.vrtl_.eval();
        if (root.state != 5 || root.nested || root.__VprimaryPorts.flag || root.__VprimaryPorts.port[2])
            return 1;
    }
    return 0;
}
