#pragma once

#include "targetdictionary.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>

// Read physical storage without TDentry::read_data or its bit-index helpers.
// Keeping this observer separate catches a shared permutation in mask creation
// and the generated differential API, including padding and array neighbours.
struct FiappStorage
{
    struct Bit
    {
        unsigned word;
        unsigned offset;
    };
    std::vector<std::uint64_t> words;
    std::vector<Bit> bits;
};

template <typename T>
struct FiappWide : std::false_type
{
};

template <std::size_t N>
struct FiappWide<VlWide<N>> : std::true_type
{
};

template <typename T>
void append_fiapp_storage(const T &data, unsigned width, FiappStorage &result)
{
    if constexpr (std::is_integral_v<T>)
    {
        const auto word = static_cast<unsigned>(result.words.size());
        result.words.push_back(static_cast<std::uint64_t>(data));
        for (unsigned bit = 0; bit < width; ++bit)
            result.bits.push_back({ word, bit });
    }
    else if constexpr (FiappWide<T>::value)
    {
        const auto first = static_cast<unsigned>(result.words.size());
        for (const auto word : data.m_storage)
            result.words.push_back(word);
        for (unsigned bit = 0; bit < width; ++bit)
            result.bits.push_back({ first + bit / 32, bit % 32 });
    }
    else
    {
        for (const auto &element : data.m_storage)
            append_fiapp_storage(element, width, result);
    }
}

template <typename T>
bool try_fiapp_storage(const vrtlfi::td::TDentry &target, FiappStorage &result)
{
    if (const auto *entry = dynamic_cast<const vrtlfi::td::Named_TDentry<T> *>(&target))
    {
        append_fiapp_storage(entry->data_, target.onedimbits_, result);
        return true;
    }
    return false;
}

inline FiappStorage read_fiapp_storage(const vrtlfi::td::TDentry &target)
{
    FiappStorage result;
    // These are the physical types in the RTL fixture, including partially
    // scalarized arrays. Expected bit locations are obtained by walking storage,
    // independently of the API's map_bit implementation.
    const bool found = try_fiapp_storage<CData>(target, result) || try_fiapp_storage<SData>(target, result) ||
                       try_fiapp_storage<IData>(target, result) || try_fiapp_storage<QData>(target, result) ||
                       try_fiapp_storage<VlWide<3>>(target, result) ||
                       try_fiapp_storage<VlUnpacked<CData, 2>>(target, result) ||
                       try_fiapp_storage<VlUnpacked<CData, 3>>(target, result) ||
                       try_fiapp_storage<VlUnpacked<VlUnpacked<CData, 2>, 2>>(target, result) ||
                       try_fiapp_storage<VlUnpacked<VlUnpacked<CData, 3>, 2>>(target, result) ||
                       try_fiapp_storage<VlUnpacked<VlWide<3>, 2>>(target, result) ||
                       try_fiapp_storage<VlUnpacked<VlWide<3>, 3>>(target, result) ||
                       try_fiapp_storage<VlUnpacked<VlUnpacked<VlWide<3>, 2>, 2>>(target, result) ||
                       try_fiapp_storage<VlUnpacked<VlUnpacked<VlWide<3>, 3>, 2>>(target, result);
    if (!found || result.bits.size() != target.bits_)
        throw std::runtime_error("Unexpected fiapp storage for " + target.get_name());
    return result;
}

inline bool check_fiapp_fault(vrtlfi::td::TDentry &fault, const vrtlfi::td::TDentry &reference, unsigned bit,
                              std::ostream &out = std::cout)
{
    const auto actual = read_fiapp_storage(fault);
    auto expected = read_fiapp_storage(reference);
    const auto location = expected.bits.at(bit);
    expected.words.at(location.word) ^= std::uint64_t{ 1 } << location.offset;
    std::vector<int> counters(expected.words.size(), 0);
    counters.at(location.word) = 1;

    std::vector<bool> logical;
    for (const auto position : expected.bits)
        logical.push_back((expected.words[position.word] >> position.offset) & 1U);
    if (actual.words != expected.words || fault.get_cntr() != counters || fault.read_data() != logical)
    {
        out << "Wrong physical bit, neighbouring element, padding, readback or counter for " << fault.get_name()
            << " bit " << bit << '\n';
        return false;
    }
    return true;
}
