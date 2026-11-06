#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "targetdictionary.hpp"

namespace
{
struct Position
{
    unsigned word;
    unsigned offset;
};

template <class Base>
void physical_words(Base &, std::vector<Base *> &);
template <class Base, std::size_t Words>
void physical_words(VlWide<Words> &, std::vector<Base *> &);
template <class Base, class Value, std::size_t Size>
void physical_words(VlUnpacked<Value, Size> &, std::vector<Base *> &);

template <class Base>
void physical_words(Base &value, std::vector<Base *> &words)
{
    words.push_back(&value);
}

template <class Base, std::size_t Words>
void physical_words(VlWide<Words> &value, std::vector<Base *> &words)
{
    for (unsigned i = 0; i < Words; ++i)
        words.push_back(&value[i]);
}

template <class Base, class Value, std::size_t Size>
void physical_words(VlUnpacked<Value, Size> &value, std::vector<Base *> &words)
{
    for (unsigned i = 0; i < Size; ++i)
        physical_words<Base>(value[i], words);
}

// Enumerate physical storage in order, visiting only valid bits of each leaf.
// This oracle deliberately avoids the runtime's logical-index division formulas.
std::vector<Position> native_positions(unsigned words, unsigned width)
{
    std::vector<Position> positions;
    for (unsigned word = 0; word < words; ++word)
        for (unsigned bit = 0; bit < width; ++bit)
            positions.push_back({ word, bit });
    return positions;
}

std::vector<Position> wide_positions(unsigned leaves, unsigned width)
{
    std::vector<Position> positions;
    unsigned physical_word = 0;
    for (unsigned leaf = 0; leaf < leaves; ++leaf)
        for (unsigned word = 0; word < (width + 31) / 32; ++word, ++physical_word)
            for (unsigned bit = 0; bit < 32; ++bit)
                if (word * 32 + bit < width)
                    positions.push_back({ physical_word, bit });
    return positions;
}

unsigned cases = 0;
unsigned tested_bits = 0;

template <class Base, class Target>
void check(Target &target, const std::vector<Position> &positions, const std::string &name)
{
    std::vector<Base *> data, mask, value;
    physical_words<Base>(target.data_, data);
    physical_words<Base>(target.mask_, mask);
    physical_words<Base>(target.assign_value_, value);
    std::vector<std::uint64_t> active(data.size(), 0), baseline(data.size(), 0);
    for (auto position : positions)
        active.at(position.word) |= std::uint64_t{ 1 } << position.offset;
    // Padding starts at one, valid data at zero: injections must preserve the
    // padding, while read_data() must omit it from the logical bit vector.
    for (unsigned word = 0; word < data.size(); ++word)
        baseline[word] = std::uint64_t(std::numeric_limits<Base>::max()) & ~active[word];

    auto physical = [&](const std::vector<Base *> &actual, const std::vector<std::uint64_t> &expected,
                        const char *operation, unsigned bit)
    {
        for (unsigned word = 0; word < actual.size(); ++word)
            if (std::uint64_t(*actual[word]) != expected[word])
                throw std::runtime_error(name + " " + operation + " logical bit=" + std::to_string(bit) +
                                         " physical word=" + std::to_string(word) + " expected=" +
                                         std::to_string(expected[word]) + " actual=" + std::to_string(*actual[word]));
    };
    auto counters = [&](unsigned selected_word, int expected_count, unsigned bit)
    {
        const auto actual = target.get_cntr();
        if (actual.size() != data.size())
            throw std::runtime_error(name + " counter length");
        for (unsigned word = 0; word < actual.size(); ++word)
            if (actual[word] != (word == selected_word ? expected_count : 0))
                throw std::runtime_error(name + " counter logical bit=" + std::to_string(bit) +
                                         " physical word=" + std::to_string(word));
    };
    for (unsigned bit = 0; bit < positions.size(); ++bit)
    {
        target.disarm();
        target.reset_mask();
        target.reset_assign_value();
        target.reset_cntr();
        for (unsigned word = 0; word < data.size(); ++word)
            *data[word] = static_cast<Base>(baseline[word]);
        const auto position = positions[bit];
        std::vector<std::uint64_t> selected(data.size(), 0), zeros(data.size(), 0), injected = baseline;
        selected[position.word] = std::uint64_t{ 1 } << position.offset;
        target.set_maskBit(bit);
        physical(mask, selected, "mask", bit);
        target.set_value_bit(bit);
        physical(value, selected, "set-value", bit);
        target.reset_value_bit(bit);
        physical(value, zeros, "reset-value", bit);
        target.set_value_bit(bit);
        target.reset_assign_value();
        physical(value, zeros, "reset-all-values", bit);

        target.inject_synchronous();
        physical(data, baseline, "disarmed", bit);
        counters(position.word, 0, bit);
        target.arm();
        target.inject_synchronous();
        injected[position.word] ^= selected[position.word];
        physical(data, injected, "inject/padding", bit);
        const auto read = target.read_data();
        if (read.size() != positions.size())
            throw std::runtime_error(name + " read length");
        for (unsigned index = 0; index < read.size(); ++index)
            if (read[index] != (index == bit))
                throw std::runtime_error(name + " read logical bit=" + std::to_string(bit) +
                                         " index=" + std::to_string(index));
        counters(position.word, 1, bit);
        target.inject_synchronous();
        physical(data, injected, "already-injected", bit);
        counters(position.word, 1, bit);
        target.reset_cntr();
        counters(position.word, 0, bit);
        target.inject_synchronous();
        physical(data, baseline, "inject-after-counter-reset", bit);
        counters(position.word, 1, bit);
        target.reset_mask();
        physical(mask, zeros, "reset-mask", bit);
    }
    ++cases;
    tested_bits += positions.size();
}

template <class Base, unsigned Width>
void native_cases(const char *type)
{
    using namespace vrtlfi::td;
    const std::string name = std::string(type) + " width=" + std::to_string(Width);
    Base scalar{};
    ZeroD_TDentry<Base> scalar_target("scalar", scalar, Width, Width);
    check<Base>(scalar_target, native_positions(1, Width), name + " 0D");
    VlUnpacked<Base, 5> one{};
    OneD_TDentry<decltype(one), Base, 5> one_target("one", one, 5 * Width, Width);
    check<Base>(one_target, native_positions(5, Width), name + " 1D[5]");
    VlUnpacked<VlUnpacked<Base, 3>, 2> two{};
    TwoD_TDentry<decltype(two), Base, 2, 3> two_target("two", two, 6 * Width, Width);
    check<Base>(two_target, native_positions(6, Width), name + " 2D[2][3]");
    VlUnpacked<VlUnpacked<VlUnpacked<Base, 4>, 3>, 2> three{};
    ThreeD_TDentry<decltype(three), Base, 2, 3, 4> three_target("three", three, 24 * Width, Width);
    check<Base>(three_target, native_positions(24, Width), name + " 3D[2][3][4]");
}

template <unsigned Width>
void wide_cases()
{
    using namespace vrtlfi::td;
    constexpr unsigned Words = (Width + 31) / 32;
    const std::string name = "wide width=" + std::to_string(Width);
    // A VlWide word dimension contributes one runtime storage dimension.
    VlWide<Words> scalar{};
    OneD_TDentry<decltype(scalar), EData, Words> scalar_target("scalar", scalar, Width, Width);
    check<EData>(scalar_target, wide_positions(1, Width), name + " 1D");
    VlUnpacked<VlWide<Words>, 5> one{};
    TwoD_TDentry<decltype(one), EData, 5, Words> one_target("one", one, 5 * Width, Width);
    check<EData>(one_target, wide_positions(5, Width), name + " 2D[5]");
    VlUnpacked<VlUnpacked<VlWide<Words>, 3>, 2> two{};
    ThreeD_TDentry<decltype(two), EData, 2, 3, Words> two_target("two", two, 6 * Width, Width);
    check<EData>(two_target, wide_positions(6, Width), name + " 3D[2][3]");
}
} // namespace

int main()
{
    try
    {
        native_cases<CData, 1>("CData");
        native_cases<CData, 3>("CData");
        native_cases<CData, 8>("CData");
        native_cases<SData, 9>("SData");
        native_cases<SData, 16>("SData");
        native_cases<IData, 17>("IData");
        native_cases<IData, 32>("IData");
        native_cases<QData, 33>("QData");
        native_cases<QData, 64>("QData");
        wide_cases<65>();
        wide_cases<66>();
        wide_cases<96>();
        wide_cases<97>();
        std::cout << "PASS " << cases << " target layouts, " << tested_bits << " logical bit positions\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
