#include "Vhelperdest_vrtlmodapi.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

int main()
{
    VhelperdestVRTLmodAPI api;
    const std::array<std::uint32_t, 3> src0_words{ 0x12345678U, 0xabcdef01U, 0x13579bdfU };
    const std::array<std::uint32_t, 3> src1_words{ 0x87654321U, 0x10fedcbaU, 0xfdb97531U };
    const std::array<std::uint32_t, 3> bit_words{ 1, 0, 0 };
    const std::array<std::uint32_t, 3> zero_words{};
    // The helper writes element [1] of one and [1][0] of two; each is 96 bits.
    const std::map<std::string, unsigned> injected_bits{
        { "TOP.clean_dst", 1 }, { "TOP.cond_dst", 1 }, { "TOP.bit_dst", 0 },
        { "TOP.wide_dst", 1 },  { "TOP.one", 97 },     { "TOP.two", 193 },
    };
    if (api.td_.size() != injected_bits.size())
    {
        std::cerr << "Wrong number of helper destinations\n";
        return 1;
    }
    for (const auto &[name, bit] : injected_bits)
        if (!api.td_.count(name) || !api.td_.at(name)->injectable_)
        {
            std::cerr << "Missing writable destination " << name << '\n';
            return 1;
        }

    api.vrtl_.eval();
    const auto &state = *api.vrtl_.rootp;
    const auto same_words = [](const auto &actual, const auto &expected)
    {
        for (unsigned word = 0; word < 3; ++word)
            if (actual[word] != expected[word])
                return false;
        return true;
    };
    if (!same_words(state.clean_dst, state.src0) || !same_words(state.cond_dst, state.src0) ||
        !same_words(state.wide_dst, state.src0) || !same_words(state.__Vtemp[0][1][0], state.src1) ||
        state.bit_dst[0] != 1 || state.bit_dst[1] != 0 || state.bit_dst[2] != 0)
    {
        std::cerr << "Helper calls did not produce their independent expected values\n";
        return 1;
    }
    for (unsigned first = 0; first < 2; ++first)
        for (unsigned word = 0; word < 3; ++word)
        {
            if (state.one[first][word] != (first == 1 ? state.src0[word] : state.src1[word]))
                return 1;
            for (unsigned second = 0; second < 2; ++second)
                if (state.two[first][second][word] !=
                    (second == 0 ? (first == 1 ? state.src0[word] : state.src1[word]) : 0))
                    return 1;
        }

    std::map<std::string, std::vector<bool>> baseline;
    for (const auto &[name, entry] : api.td_)
        baseline[name] = entry->read_data();

    for (const auto &[name, bit] : injected_bits)
    {
        for (const auto &[unused, entry] : api.td_)
        {
            entry->disarm();
            entry->reset_mask();
            entry->reset_cntr();
        }
        auto &target = *api.td_.at(name);
        if (api.prep_inject(target, bit) != 0)
            return 1;
        target.arm();
        api.vrtl_.eval();

        // Check physical storage independently of read_data and API bit mapping.
        // Every selected wide leaf uses bit 1 of word 0; bit_dst uses bit 0.
        const auto expected_words = [&](const char *target_name, auto words)
        {
            if (name == target_name)
                words[0] ^= name == "TOP.bit_dst" ? 1U : 2U;
            return words;
        };
        if (!same_words(state.clean_dst, expected_words("TOP.clean_dst", src0_words)) ||
            !same_words(state.cond_dst, expected_words("TOP.cond_dst", src0_words)) ||
            !same_words(state.bit_dst, expected_words("TOP.bit_dst", bit_words)) ||
            !same_words(state.wide_dst, expected_words("TOP.wide_dst", src0_words)))
        {
            std::cerr << "Wrong physical scalar words after injecting " << name << '\n';
            return 1;
        }
        for (unsigned first = 0; first < 2; ++first)
        {
            auto expected_one = first == 1 ? expected_words("TOP.one", src0_words) : src1_words;
            if (!same_words(state.one[first], expected_one))
            {
                std::cerr << "Wrong physical one[" << first << "] words after injecting " << name << '\n';
                return 1;
            }
            for (unsigned second = 0; second < 2; ++second)
            {
                auto expected_two =
                    second == 0 ? (first == 1 ? expected_words("TOP.two", src0_words) : src1_words) : zero_words;
                if (!same_words(state.two[first][second], expected_two))
                {
                    std::cerr << "Wrong physical two[" << first << "][" << second << "] words after injecting " << name
                              << '\n';
                    return 1;
                }
            }
        }

        for (const auto &[other_name, entry] : api.td_)
        {
            auto expected = baseline.at(other_name);
            if (other_name == name)
                expected[bit] = !expected[bit];
            const auto counts = entry->get_cntr();
            const auto count = std::accumulate(counts.begin(), counts.end(), 0);
            if (entry->read_data() != expected || count != (other_name == name ? 1 : 0))
            {
                std::cerr << "Injection into " << name << " changed the wrong data/count for " << other_name << '\n';
                return 1;
            }
        }
        if (state.index != 1 || state.bit_source != 1 || state.src0[0] != 0x12345678U || state.src0[1] != 0xabcdef01U ||
            state.src0[2] != 0x13579bdfU || state.src1[0] != 0x87654321U || state.src1[1] != 0x10fedcbaU ||
            state.src1[2] != 0xfdb97531U)
        {
            std::cerr << "A read-only helper source or index was modified\n";
            return 1;
        }
    }
    return 0;
}
