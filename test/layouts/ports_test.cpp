#include "Vport_types_vrtlmod_diffapi.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

// The differential API must preserve source-level port value types, including
// bool rather than Clang's canonical _Bool spelling and nested namespaces.
using Diff = Vport_typesVRTLmodAPIDifferential;
static_assert(std::is_same_v<decltype(std::declval<Diff>().r_diff_), bool>);
static_assert(std::is_same_v<decltype(std::declval<Diff>().o_diff_), bool>);
static_assert(std::is_same_v<decltype(std::declval<Diff>().e_diff_), std::uint32_t>);
static_assert(std::is_same_v<decltype(std::declval<Diff>().t_diff_), std::uint32_t>);
static_assert(std::is_same_v<decltype(std::declval<Diff>().wide_diff_), sc_dt::sc_bv<65>>);

int sc_main(int, char**)
{
    Vport_typesVRTLmodAPI api("ports");
    sc_core::sc_signal<bool> clk, r, o;
    sc_core::sc_signal<std::uint32_t> e, t;
    sc_core::sc_signal<sc_dt::sc_bv<65>> wide;
    api.vrtl_.clk(clk);
    api.vrtl_.r(r);
    api.vrtl_.e(e);
    api.vrtl_.o(o);
    api.vrtl_.t(t);
    api.vrtl_.wide(wide);
    clk = false;
    r = false;
    e = 0;
    auto tick = [&] {
        clk = false;
        sc_core::sc_start(1, sc_core::SC_NS);
        clk = true;
        sc_core::sc_start(1, sc_core::SC_NS);
    };
    tick();

    const std::array<const char*, 3> names{ "state_bit", "state_word", "state_wide" };
    const std::array<unsigned, 3> bits{ 1, 32, 65 };
    const std::array<unsigned, 3> flip{ 0, 31, 64 };
    for (unsigned index = 0; index < names.size(); ++index)
    {
        vrtlfi::td::TDentry* target = nullptr;
        for (const auto& [name, entry] : api.td_)
        {
            auto logical_name = name.substr(name.find_last_of('.') + 1);
            const auto dot = logical_name.rfind("__DOT__");
            if (dot != std::string::npos)
                logical_name.erase(0, dot + 7);
            if (logical_name.compare(0, 7, "__PVT__") == 0)
                logical_name.erase(0, 7);
            if (entry->injectable_ && logical_name == names[index])
            {
                if (target)
                    return 1;
                target = entry;
            }
        }
        if (!target || target->bits_ != bits[index])
        {
            std::cerr << "Missing clocked state " << names[index] << '\n';
            return 1;
        }
        target->reset_mask();
        if (api.prep_inject(*target, flip[index]) != 0)
            return 1;
        target->arm();
        tick();
        if (o.read() != (index == 0) || t.read() != (index == 1 ? 0x80000000U : 0U)
            || wide.read().get_bit(64) != (index == 2) || wide.read().range(63, 0).to_uint64() != 0)
        {
            std::cerr << "Clocked injection reached wrong SystemC storage: " << names[index] << '\n';
            return 1;
        }
        const auto counters = target->get_cntr();
        if (counters.size() != (bits[index] + 31) / 32)
            return 1;
        for (unsigned word = 0; word < counters.size(); ++word)
            if (counters[word] != (word == flip[index] / 32 ? 1 : 0))
                return 1;
        target->disarm();
        tick();
        if (o.read() || t.read() || wide.read().or_reduce())
            return 1;
    }
    return 0;
}
