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
/// @file targetdictionary.hpp
/// @date 2022-08-18
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __VRTLFI_TD_TARGETDICTIONARY_HPP__
#define __VRTLFI_TD_TARGETDICTIONARY_HPP__

#include <vector>
#include <memory>

#include <verilated.h>

#include <map>
#include <cstring>
#include <stdexcept>

#define __LIKELY(x) __builtin_expect(!!(x), 1)
#define __UNLIKELY(x) __builtin_expect(!!(x), 0)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for all vrtl-fi
namespace vrtlfi
{

typedef enum INJ_TYPE
{
    BIASED_S,
    BIASED_R,
    BITFLIP,
    ASSIGN
} INJ_TYPE_t;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief namespace for target dictionary
namespace td
{

// FORWARDS ////////////////////////////////////////////////////////////////////////////////////////
template <typename>
class Named_TDentry;

template <typename>
class ZeroD_TDentry;

template <typename, typename, int>
class OneD_TDentry;

template <typename, typename, int, int>
class TwoD_TDentry;

template <typename, typename, int, int, int>
class ThreeD_TDentry;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @class TDentry
/// @brief fault injection target dictionary entry. Pure abstract base class!
class TDentry
{
  public:
    bool enable_;               ///< Entry is enabled to perform injections
    INJ_TYPE_t inj_type_;       ///< Type of injection to perform
    const unsigned bits_;       ///< Number of bits within target
    const unsigned onedimbits_; ///< Number of bits of one-dimensional element (e.g. only 65 bits of a target
                                ///< represented by 3*32-bit words)

  public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief arm for injection
    void arm(void) { enable_ = true; }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief disarm for injection
    void disarm(void) { enable_ = false; }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief set masking bit
    /// \param bit index of mask bit to be set, 0:lsb
    virtual void set_maskBit(unsigned bit) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief set value bit
    /// \param bit index of mask bit to be set, 0:lsb
    virtual void set_value_bit(unsigned bit) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief reset value bit
    /// \param bit index of mask bit to be set, 0:lsb
    virtual void reset_value_bit(unsigned bit) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief reset complete mask
    virtual void reset_mask(void) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief reset injection value mask (used for INJ_TYPE::ASSIGN)
    virtual void reset_assign_value(void) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Returns the target compressed into a bit-vector of length bits_
    virtual std::vector<bool> read_data(void) const = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief entry identifier name
    virtual const std::string &get_name() const = 0;

    virtual void inject_on_update(std::initializer_list<unsigned int> i = {}) = 0;
    virtual void inject_synchronous(void) = 0;
    virtual void incr_cntr(std::initializer_list<unsigned int> i = {}) = 0;
    virtual void decr_cntr(std::initializer_list<unsigned int> i = {}) = 0;
    virtual void reset_cntr(std::initializer_list<unsigned int> i = {}) = 0;
    virtual std::vector<int> get_cntr(void) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    TDentry(unsigned bits, unsigned onedimbits)
        : enable_(false), inj_type_(INJ_TYPE::BITFLIP), bits_(bits), onedimbits_(onedimbits)
    {
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Destructor
    virtual ~TDentry(void) {}
};

template <typename vcontainer_t>
class Named_TDentry : public TDentry
{
    std::string name_;

  public:
    vcontainer_t &data_;          ///< Reference to VRTL signal
    vcontainer_t mask_{};         ///< Shadow of VRTL signal holding injection bits
    vcontainer_t assign_value_{}; ///< Shadow of VRTL signal holding injection value according to masked bits

  public:
    virtual void set_maskBit(unsigned bit) {}
    virtual void reset_mask(void) {}

    virtual void set_value_bit(unsigned bit) {}
    virtual void reset_value_bit(unsigned bit) {}
    virtual void reset_assign_value(void) {}

    virtual std::vector<bool> read_data(void) const { return std::vector<bool>{}; }
    // virtual void inject(int word = 0){};
    virtual void inject_on_update(std::initializer_list<unsigned int> i = {}) {}
    virtual void inject_synchronous(void) {}
    virtual void incr_cntr(std::initializer_list<unsigned int> i = {}) {}
    virtual void decr_cntr(std::initializer_list<unsigned int> i = {}) {}
    virtual std::vector<int> get_cntr(void) { return {}; }

    const std::string &get_name() const { return name_; }
    Named_TDentry(const char *name, vcontainer_t &data, unsigned bits, unsigned onedimbits)
        : TDentry(bits, onedimbits), name_(name), data_(data)
    {
    }
    virtual ~Named_TDentry() {}
};

template <typename vcontainer_t>
class ZeroD_TDentry final : public Named_TDentry<vcontainer_t>
{
    using BASE = Named_TDentry<vcontainer_t>;

  protected:
    const int BASETYPE_BITS{ sizeof(vcontainer_t) * 8 };

    int cntr_{}; ///< injection cntr. increments on each performed injection until 0

    void inject(void);

  public:
  public:
    void __inject_on_update(void) { inject(); }
    void __incr_cntr(void) { ++cntr_; }
    void __decr_cntr(void) { --cntr_; }
    void __reset_cntr(void) { cntr_ = 0; }

    // TDentry interface methods:
    void set_maskBit(unsigned bit) override { BASE::mask_ |= (1 << bit); }
    void reset_mask(void) override { BASE::mask_ = 0; }

    virtual void set_value_bit(unsigned bit) override { BASE::assign_value_ |= (1 << bit); }
    virtual void reset_value_bit(unsigned bit) override { BASE::assign_value_ &= ~(1 << bit); }
    virtual void reset_assign_value(void) override { BASE::assign_value_ = 0; }

    std::vector<bool> read_data(void) const override;
    void inject_on_update(std::initializer_list<unsigned int> i = {}) override { __inject_on_update(); }
    void inject_synchronous(void) { inject(); }
    void incr_cntr(std::initializer_list<unsigned int> i = {}) override { __incr_cntr(); }
    void decr_cntr(std::initializer_list<unsigned int> i = {}) override { __decr_cntr(); }
    void reset_cntr(std::initializer_list<unsigned int> i = {}) override { __reset_cntr(); }
    std::vector<int> get_cntr(void) override { return std::vector<int>{ cntr_ }; }

    ZeroD_TDentry(const char *name, vcontainer_t &data, unsigned bits, unsigned onedimbits)
        : BASE(name, data, bits, onedimbits)
    {
    }
    virtual ~ZeroD_TDentry() {}
};

template <typename vcontainer_t, typename vbasetype_t, int M>
class OneD_TDentry final : public Named_TDentry<vcontainer_t>
{
    using BASE = Named_TDentry<vcontainer_t>;

  protected:
    const int BASETYPE_BITS{ sizeof(vbasetype_t) * 8 };

    int cntr_[M]{}; ///< injection cntr. increments on each performed injection until 0

    constexpr std::array<unsigned, 2> map_bit(unsigned bit);

    void inject(unsigned m);

  public:
    void __inject_on_update(unsigned m) { inject(m); }
    void __incr_cntr(unsigned m) { ++cntr_[m]; }
    void __decr_cntr(unsigned m) { --cntr_[m]; }
    void __reset_cntr(unsigned m) { cntr_[m] = 0; }

    // TDentry interface methods:
    void set_maskBit(unsigned bit) override;
    void reset_mask(void) override;

    virtual void set_value_bit(unsigned bit) override;
    virtual void reset_value_bit(unsigned bit) override;
    virtual void reset_assign_value(void) override;

    std::vector<bool> read_data(void) const override;
    void inject_on_update(std::initializer_list<unsigned int> i = {}) override;
    void inject_synchronous(void) override;
    void incr_cntr(std::initializer_list<unsigned int> i = {}) override;
    void decr_cntr(std::initializer_list<unsigned int> i = {}) override;
    void reset_cntr(std::initializer_list<unsigned int> i = {}) override;
    std::vector<int> get_cntr(void) override;

    OneD_TDentry(const char *name, vcontainer_t &data, unsigned bits, unsigned onedimbits)
        : BASE(name, data, bits, onedimbits)
    {
    }
    virtual ~OneD_TDentry(void) {}
};

template <typename vcontainer_t, typename vbasetype_t, int L, int M>
class TwoD_TDentry final : public Named_TDentry<vcontainer_t>
{
    using BASE = Named_TDentry<vcontainer_t>;

  protected:
    const int BASETYPE_BITS{ sizeof(vbasetype_t) * 8 };

    int cntr_[L][M]{}; ///< injection cntr. increments on each performed injection until 0

    constexpr std::array<unsigned, 3> map_bit(unsigned bit);

    void inject(unsigned l, unsigned m);

  public:
    void __inject_on_update(unsigned l, unsigned m) { inject(l, m); }
    void __incr_cntr(unsigned l, unsigned m) { ++cntr_[l][m]; }
    void __decr_cntr(unsigned l, unsigned m) { --cntr_[l][m]; }
    void __reset_cntr(unsigned l, unsigned m) { cntr_[l][m] = 0; }

    // TDentry interface methods:
    void set_maskBit(unsigned bit) override;
    void reset_mask(void) override;

    virtual void set_value_bit(unsigned bit) override;
    virtual void reset_value_bit(unsigned bit) override;
    virtual void reset_assign_value(void) override;

    std::vector<bool> read_data(void) const override;
    void inject_on_update(std::initializer_list<unsigned int> i = {}) override;
    void inject_synchronous(void) override;
    void incr_cntr(std::initializer_list<unsigned int> i = {}) override;
    void decr_cntr(std::initializer_list<unsigned int> i = {}) override;
    void reset_cntr(std::initializer_list<unsigned int> i = {}) override;
    std::vector<int> get_cntr(void) override;

    TwoD_TDentry(const char *name, vcontainer_t &data, const unsigned bits, const unsigned onedimbits)
        : BASE(name, data, bits, onedimbits)
    {
    }
    virtual ~TwoD_TDentry(void) {}
};

template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
class ThreeD_TDentry final : public Named_TDentry<vcontainer_t>
{
    using BASE = Named_TDentry<vcontainer_t>;

  protected:
    const int BASETYPE_BITS{ sizeof(vbasetype_t) * 8 };

    int cntr_[K][L][M]{}; ///< injection cntr. increments on each performed injection until 0

    constexpr std::array<unsigned, 4> map_bit(unsigned bit);

    void inject(unsigned k, unsigned l, unsigned m);

  public:
    void __inject_on_update(unsigned k, unsigned l, unsigned m) { inject(k, l, m); }
    void __incr_cntr(unsigned k, unsigned l, unsigned m) { ++cntr_[k][l][m]; }
    void __decr_cntr(unsigned k, unsigned l, unsigned m) { --cntr_[k][l][m]; }
    void __reset_cntr(unsigned k, unsigned l, unsigned m) { cntr_[k][l][m] = 0; }

    // TDentry interface methods:
    void set_maskBit(unsigned bit) override;
    void reset_mask(void) override;

    virtual void set_value_bit(unsigned bit) override;
    virtual void reset_value_bit(unsigned bit) override;
    virtual void reset_assign_value(void) override;

    std::vector<bool> read_data(void) const override;

    void inject_on_update(std::initializer_list<unsigned int> i = {}) override;
    void inject_synchronous(void) override;
    void incr_cntr(std::initializer_list<unsigned int> i = {}) override;
    void decr_cntr(std::initializer_list<unsigned int> i = {}) override;
    void reset_cntr(std::initializer_list<unsigned int> i = {}) override;
    std::vector<int> get_cntr(void) override;

    ThreeD_TDentry(const char *name, vcontainer_t &data, const unsigned bits, const unsigned onedimbits)
        : BASE(name, data, bits, onedimbits)
    {
    }
    virtual ~ThreeD_TDentry(void) {}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @class TD_API
/// @brief fault injection target dictionary. Pure abstract!
class TD_API
{
  public:
    ///////////////////////////////////////////////////////////////////////
    /// \brief Return codes for API methods
    typedef enum BIT_CODES
    {
        ERROR_TARGET_NAME_UNKNOWN = 0x0001,
        ERROR_TARGET_IDX_UNKNOWN = 0x0002,
        ERROR_BIT_OUTOFRANGE = 0x0004,
        ERROR_INJTYPE_UNSUPPORTED = 0x0008,
        SUCC_TARGET_ARMED = 0x10,
        SUCC_TARGET_DISARMED = 0x20,
        GENERIC_OK = 0,
        GENERIC_ERROR = 0x8000
    } BIT_CODES_t;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief List of TDentry dictionary entries
    std::map<std::string, std::shared_ptr<TDentry>> td_{};

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Prepare an injection: Set bits accordingly and arm target
    /// \param targetname string identifier name of injection target
    /// \param type injection type
    /// \return BIT_CODES
    int prep_inject(const std::string &targetname, const unsigned bit, const INJ_TYPE_t type = BITFLIP)
    {
        return prep_inject(targetname.c_str(), bit, type);
    }
    int prep_inject(const char *targetname, const unsigned bit, const INJ_TYPE_t type = BITFLIP)
    {
        if (TDentry *tptr = get_target(targetname))
        {
            return prep_inject(*tptr, bit, type);
        }
        return BIT_CODES::ERROR_TARGET_NAME_UNKNOWN;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Prepare an injection: Set bits accordingly and arm target
    /// \param target string identifier name of injection target
    /// \param type injection type
    /// \return BIT_CODES
    int prep_inject(TDentry &target, const unsigned bit, const INJ_TYPE_t type = BITFLIP)
    {
        int ret = 0;
        if (type != INJ_TYPE::BITFLIP)
        {
            ret |= BIT_CODES::ERROR_INJTYPE_UNSUPPORTED;
        }
        try
        {
            if (bit > target.bits_ - 1)
            {
                return BIT_CODES::ERROR_BIT_OUTOFRANGE;
            }
            target.inj_type_ = type;
            target.set_maskBit(bit);
            target.reset_cntr();
        }
        catch (const std::out_of_range &e)
        {
            return BIT_CODES::ERROR_TARGET_IDX_UNKNOWN;
        }
        return BIT_CODES::GENERIC_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Prepare an injection: Set bits accordingly and arm target
    /// \param target injection target
    /// \param type injection type
    /// \param bits vector of bit numbers to be set in injection mask
    /// \return BIT_CODES
    int prep_inject(TDentry &target, const std::vector<unsigned> &bits, const INJ_TYPE_t type = BITFLIP) const
    {
        int ret = 0;
        if (type != INJ_TYPE::BITFLIP)
        {
            ret |= BIT_CODES::ERROR_INJTYPE_UNSUPPORTED;
        }
        for (const auto &bit : bits)
        {
            if (bit > target.bits_ - 1)
            {
                return BIT_CODES::ERROR_BIT_OUTOFRANGE;
            }
            target.set_maskBit(bit);
        }
        target.inj_type_ = type;
        target.reset_cntr();
        return BIT_CODES::GENERIC_OK;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Prepare an injection: Set bits for value mask accordingly and arm target
    /// \param target injection target
    /// \param value_map map of bit numbers (key) and value to be assigned (0-> reset keyed bit, 1->set)
    /// \return BIT_CODES
    int prep_value_inject(TDentry &target, const std::map<unsigned, bool> &value_map) const
    {
        int ret = 0;
        target.reset_mask();
        target.reset_assign_value();
        for (const auto &it : value_map)
        {
            unsigned bit = it.first;
            bool value = it.second;
            if (bit > target.bits_ - 1)
            {
                // return BIT_CODES::ERROR_BIT_OUTOFRANGE;
                continue; // just skip this bit that is out of range
            }
            target.set_maskBit(bit);
            if (value)
            {
                target.set_value_bit(bit);
            }
            else
            {
                target.reset_value_bit(bit);
            }
        }
        target.inj_type_ = INJ_TYPE::ASSIGN;
        target.reset_cntr();
        return BIT_CODES::GENERIC_OK;
    }

    TDentry *get_target(const char *targetname) const
    {
        try
        {
            return td_.at(targetname).get();
        }
        catch (const std::out_of_range &e)
        {
            return nullptr;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Reset all injection settings for a target
    /// \param targetname string identifier name of injection target
    /// \return BIT_CODES
    int reset_inject(const char *targetname)
    {
        try
        {
            auto &x = td_.at(targetname);
            x->enable_ = false;
            x->reset_cntr();
            x->reset_mask();
            return BIT_CODES::SUCC_TARGET_DISARMED;
        }
        catch (const std::out_of_range &e)
        {
            return BIT_CODES::ERROR_TARGET_IDX_UNKNOWN;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Get index of entry within entry vector
    /// \param targetname string identifier name of injection target
    /// \return index of injection entry (-1 if not found)
    int get_EntryArrayIndex(const char *targetname) const
    {
        unsigned int i = 0;
        for (auto const &it : td_)
        {
            if (it.second->get_name() == targetname)
            {
                return i;
            }
            ++i;
        }
        return -1;
    }

    TD_API(void) = default;
    virtual ~TD_API(void) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
// ZeroD_TDentry impl //////////////////////////////////////////////////////////////////////////////
template <typename vcontainer_t>
inline void ZeroD_TDentry<vcontainer_t>::inject(void)
{
    if (__UNLIKELY(TDentry::enable_))
    {
        if (__UNLIKELY(cntr_ <= 0))
        {
            if (__LIKELY(TDentry::inj_type_ == INJ_TYPE::BITFLIP))
            {
                BASE::data_ ^= BASE::mask_;
            }
            else if (TDentry::inj_type_ == INJ_TYPE::BIASED_S)
            {
                BASE::data_ |= BASE::mask_;
            }
            else if (TDentry::inj_type_ == INJ_TYPE::BIASED_R)
            { //  == INJ_TYPE::BIASED_R
                BASE::data_ &= ~BASE::mask_;
            }
            else
            { // == ASSIGN aka data<=mask
                BASE::data_ = BASE::mask_;
            }
            __incr_cntr();
        }
    }
}
template <typename vcontainer_t>
std::vector<bool> ZeroD_TDentry<vcontainer_t>::read_data(void) const
{
    std::vector<bool> bitstream;
    for (int bit = 0; bit < TDentry::bits_; ++bit)
    {
        vcontainer_t msk = (1 << bit);
        bitstream.push_back((BASE::data_ & msk) ? true : false);
    }
    return bitstream;
}

// Template implmenatations:
////////////////////////////////////////////////////////////////////////////////////////////////////
// OneD_TDentry impl ///////////////////////////////////////////////////////////////////////////////
template <typename vcontainer_t, typename vbasetype_t, int M>
constexpr std::array<unsigned, 2> OneD_TDentry<vcontainer_t, vbasetype_t, M>::map_bit(unsigned bit)
{
    std::array<unsigned, 2> x{};
    if (BASE::onedimbits_ <= BASETYPE_BITS)
    {                                   // array of elements, e.g., logic[1:0] x[3][5];
        x[0] = bit / BASE::onedimbits_; // M element offset
        x[1] = bit % BASE::onedimbits_; // bit offset
    }
    else
    { // basetype extender target, e.g., 65-bit vector in 3 WDatas
        x[0] = bit / BASETYPE_BITS;
        x[1] = bit % BASETYPE_BITS;
    }
    return x;
}
template <typename vcontainer_t, typename vbasetype_t, int M>
inline void OneD_TDentry<vcontainer_t, vbasetype_t, M>::inject(unsigned m)
{
    if (__UNLIKELY(TDentry::enable_))
    {
        if (__UNLIKELY(cntr_[m] <= 0) && BASE::mask_[m])
        {
            if (__LIKELY(TDentry::inj_type_ == INJ_TYPE::BITFLIP))
            {
                BASE::data_[m] ^= BASE::mask_[m];
            }
            else if (TDentry::inj_type_ == INJ_TYPE::BIASED_S)
            {
                BASE::data_[m] |= BASE::mask_[m];
            }
            else if (TDentry::inj_type_ == INJ_TYPE::BIASED_R)
            { //  == INJ_TYPE::BIASED_R
                BASE::data_[m] &= ~BASE::mask_[m];
            }
            else
            { //== ASSIGN aka data<=mask
                BASE::data_[m] &= ~(BASE::mask_[m]);
                BASE::data_[m] |= BASE::mask_[m] & BASE::assign_value_[m];
            }
            __incr_cntr(m);
        }
    }
}
template <typename vcontainer_t, typename vbasetype_t, int M>
void OneD_TDentry<vcontainer_t, vbasetype_t, M>::set_maskBit(unsigned bit)
{
    auto b = map_bit(bit);
    BASE::mask_[b[0]] |= 1 << b[1];
}
template <typename vcontainer_t, typename vbasetype_t, int M>
void OneD_TDentry<vcontainer_t, vbasetype_t, M>::reset_mask(void)
{
    for (int m = 0; m < M; ++m)
        BASE::mask_[m] = 0;
}
template <typename vcontainer_t, typename vbasetype_t, int M>
void OneD_TDentry<vcontainer_t, vbasetype_t, M>::set_value_bit(unsigned bit)
{
    auto b = map_bit(bit);
    BASE::assign_value_[b[0]] |= 1 << b[1];
}
template <typename vcontainer_t, typename vbasetype_t, int M>
void OneD_TDentry<vcontainer_t, vbasetype_t, M>::reset_value_bit(unsigned bit)
{
    auto b = map_bit(bit);
    BASE::assign_value_[b[0]] &= ~(1 << b[1]);
}
template <typename vcontainer_t, typename vbasetype_t, int M>
void OneD_TDentry<vcontainer_t, vbasetype_t, M>::reset_assign_value(void)
{
    for (int m = 0; m < M; ++m)
        BASE::assign_value_[m] = 0;
}
template <typename vcontainer_t, typename vbasetype_t, int M>
std::vector<bool> OneD_TDentry<vcontainer_t, vbasetype_t, M>::read_data(void) const
{
    // this needs some work... generally a caller to read_data only knows the total sum of bits_,
    // thus, would only provide a buffer of such a length via pData
    // read_data has to compress the variant bitvector (either basetype extender or element slices) to a minimum
    // byte-fragmented vector... -> write helper functions that one-dimensionalize multidimensional arrays and
    // compress content based on onedimbits, dims, ...
    std::vector<bool> bitstream;
    if (BASE::onedimbits_ <= BASETYPE_BITS)
    { // array of elements, e.g., logic[1:0] x[3];
        for (int element = 0; element < M; ++element)
            for (int bit = 0; bit < BASE::onedimbits_; ++bit)
            {
                vbasetype_t msk = (1 << bit);
                bitstream.push_back(BASE::data_[element] & msk ? true : false);
            }
    }
    else
    { // basetype extender target, e.g., 65-bit vector in 3 WDatas
        for (int element = 0; element < M; ++element)
        {
            if (element < (M - 1))
            {
                for (int bit = 0; bit < BASETYPE_BITS; ++bit)
                {
                    vbasetype_t msk = (1 << bit);
                    bitstream.push_back((BASE::data_[element] & msk) ? true : false);
                }
            }
            else
            {
                for (int bit = 0; bit < BASE::onedimbits_ - (M - 1) * BASETYPE_BITS; ++bit)
                {
                    vbasetype_t msk = (1 << bit);
                    bitstream.push_back((BASE::data_[element] & msk) ? true : false);
                }
            }
        }
    }
    return bitstream;
}
template <typename vcontainer_t, typename vbasetype_t, int M>
inline void OneD_TDentry<vcontainer_t, vbasetype_t, M>::inject_on_update(std::initializer_list<unsigned int> i)
{
    __inject_on_update(*(i.begin()));
}
template <typename vcontainer_t, typename vbasetype_t, int M>
inline void OneD_TDentry<vcontainer_t, vbasetype_t, M>::inject_synchronous(void)
{
    for (int m = 0; m < M; ++m)
        inject(m);
}
template <typename vcontainer_t, typename vbasetype_t, int M>
void OneD_TDentry<vcontainer_t, vbasetype_t, M>::incr_cntr(std::initializer_list<unsigned int> i)
{
    __incr_cntr(*(i.begin()));
}
template <typename vcontainer_t, typename vbasetype_t, int M>
void OneD_TDentry<vcontainer_t, vbasetype_t, M>::decr_cntr(std::initializer_list<unsigned int> i)
{
    __decr_cntr(*(i.begin()));
}
template <typename vcontainer_t, typename vbasetype_t, int M>
void OneD_TDentry<vcontainer_t, vbasetype_t, M>::reset_cntr(std::initializer_list<unsigned int> i)
{
    if (i.size() > 0)
    {
        __reset_cntr(*(i.begin()));
    }
    else
    {
        for (int m = 0; m < M; ++m)
            __reset_cntr(m);
    }
}
template <typename vcontainer_t, typename vbasetype_t, int M>
std::vector<int> OneD_TDentry<vcontainer_t, vbasetype_t, M>::get_cntr(void)
{
    std::vector<int> c;
    for (int m = 0; m < M; ++m)
        c.push_back(cntr_[m]);
    return c;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoD_TDentry impl ///////////////////////////////////////////////////////////////////////////////
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
constexpr std::array<unsigned, 3> TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::map_bit(unsigned bit)
{
    std::array<unsigned, 3> x{};
    if (BASE::onedimbits_ <= BASETYPE_BITS)
    {                                         // array of elements, e.g., logic[1:0] x[3][5];
        x[0] = bit / (BASE::onedimbits_ * M); // L element offset
        x[1] = (bit / BASE::onedimbits_) % M; // M element offset
        x[2] = bit % BASE::onedimbits_;       // bit offset
    }
    else
    { // basetype extender target, e.g., 65-bit vector in 3 WDatas
        x[0] = bit / (BASE::onedimbits_);
        x[1] = (bit - x[0] * BASE::onedimbits_) / BASETYPE_BITS;
        x[2] = bit % BASETYPE_BITS;
    }
    return x;
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
inline void TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::inject(unsigned l, unsigned m)
{
    if (__UNLIKELY(TDentry::enable_))
    {
        if (__UNLIKELY(cntr_[l][m] <= 0) && BASE::mask_[l][m])
        {
            if (__LIKELY(TDentry::inj_type_ == INJ_TYPE::BITFLIP))
            {
                BASE::data_[l][m] ^= BASE::mask_[l][m];
            }
            else if (TDentry::inj_type_ == INJ_TYPE::BIASED_S)
            {
                BASE::data_[l][m] |= BASE::mask_[l][m];
            }
            else if (TDentry::inj_type_ == INJ_TYPE::BIASED_R)
            { //  == INJ_TYPE::BIASED_R
                BASE::data_[l][m] &= ~BASE::mask_[l][m];
            }
            else
            { //== ASSIGN aka data<=mask
                BASE::data_[l][m] &= ~(BASE::mask_[l][m]);
                BASE::data_[l][m] |= BASE::mask_[l][m] & BASE::assign_value_[l][m];
            }
            __incr_cntr(l, m);
        }
    }
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
void TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::set_maskBit(unsigned bit)
{
    auto b = map_bit(bit);
    BASE::mask_[b[0]][b[1]] |= 1 << b[2];
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
void TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::reset_mask(void)
{
    for (int l = 0; l < L; ++l)
        for (int m = 0; m < M; ++m)
            BASE::mask_[l][m] = 0;
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
void TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::set_value_bit(unsigned bit)
{
    auto b = map_bit(bit);
    BASE::assign_value_[b[0]][b[1]] |= 1 << b[2];
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
void TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::reset_value_bit(unsigned bit)
{
    auto b = map_bit(bit);
    BASE::assign_value_[b[0]][b[1]] &= ~(1 << b[2]);
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
void TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::reset_assign_value(void)
{
    for (int l = 0; l < L; ++l)
        for (int m = 0; m < M; ++m)
            BASE::assign_value_[l][m] = 0;
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
std::vector<bool> TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::read_data(void) const
{
    // this needs some work... generally a caller to read_data only knows the total sum of bits_,
    // thus, would only provide a buffer of such a length via pData
    // read_data has to compress the variant bitvector (either basetype extender or element slices) to a minimum
    // byte-fragmented vector... -> write helper functions that one-dimensionalize multidimensional arrays and
    // compress content based on onedimbits, dims, ...
    std::vector<bool> bitstream;
    if (BASE::onedimbits_ <= BASETYPE_BITS)
    { // array of elements, e.g., logic[1:0] x[3];
        for (int row = 0; row < L; ++row)
            for (int element = 0; element < M; ++element)
                for (int bit = 0; bit < BASE::onedimbits_; ++bit)
                {
                    vbasetype_t msk = (1 << bit);
                    bitstream.push_back((BASE::data_[row][element] & msk) ? true : false);
                }
    }
    else
    { // basetype extender target, e.g., 65-bit vector in 3 WDatas
        for (int row = 0; row < L; ++row)
            for (int element = 0; element < M; ++element)
            {
                if (element < (M - 1))
                {
                    for (int bit = 0; bit < BASETYPE_BITS; ++bit)
                    {
                        vbasetype_t msk = (1 << bit);
                        bitstream.push_back((BASE::data_[row][element] & msk) ? true : false);
                    }
                }
                else
                {
                    for (int bit = 0; bit < BASE::onedimbits_ - (M - 1) * BASETYPE_BITS; ++bit)
                    {
                        vbasetype_t msk = (1 << bit);
                        bitstream.push_back((BASE::data_[row][element] & msk) ? true : false);
                    }
                }
            }
    }
    return bitstream;
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
inline void TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::inject_on_update(std::initializer_list<unsigned int> i)
{
    __inject_on_update(*(i.begin()), *(i.begin() + 1));
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
inline void TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::inject_synchronous(void)
{
    for (int l = 0; l < L; ++l)
        for (int m = 0; m < M; ++m)
            inject(l, m);
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
void TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::incr_cntr(std::initializer_list<unsigned int> i)
{
    __incr_cntr(*(i.begin()), *(i.begin() + 1));
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
void TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::decr_cntr(std::initializer_list<unsigned int> i)
{
    __decr_cntr(*(i.begin()), *(i.begin() + 1));
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
void TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::reset_cntr(std::initializer_list<unsigned int> i)
{
    if (i.size() > 0)
    {
        __reset_cntr(*(i.begin()), *(i.begin() + 1));
    }
    else
    {
        for (int l = 0; l < L; ++l)
            for (int m = 0; m < M; ++m)
                __reset_cntr(l, m);
    }
}
template <typename vcontainer_t, typename vbasetype_t, int L, int M>
std::vector<int> TwoD_TDentry<vcontainer_t, vbasetype_t, L, M>::get_cntr(void)
{
    std::vector<int> c;
    for (int l = 0; l < L; ++l)
        for (int m = 0; m < M; ++m)
            c.push_back(cntr_[l][m]);
    return c;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreeD_TDentry impl /////////////////////////////////////////////////////////////////////////////
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
constexpr std::array<unsigned, 4> ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::map_bit(unsigned bit)
{
    std::array<unsigned, 4> x{};
    unsigned rowbits = BASE::onedimbits_ * M;
    unsigned planebits = rowbits * L;
    if (BASE::onedimbits_ <= BASETYPE_BITS)
    {                                                // array of elements, e.g., logic[1:0] x[3][5];
        x[0] = bit / planebits;                      // plane offset
        x[1] = (bit - x[0] * planebits) / (rowbits); // L element offset
        x[2] = (bit - x[0] * planebits - x[1] * rowbits) / BASE::onedimbits_; // M element offset
        x[3] = bit % BASE::onedimbits_;                                       // bit offset
    }
    else
    { // basetype extender target, e.g., 65-bit vector in 3 WDatas
        x[0] = bit / planebits;
        x[1] = (bit - x[0] * planebits) / (rowbits);
        x[2] = (bit - x[0] * planebits - x[1] * rowbits) / BASETYPE_BITS;
        x[3] = bit % BASETYPE_BITS;
    }
    return x;
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
inline void ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::inject(unsigned k, unsigned l, unsigned m)
{
    if (__UNLIKELY(TDentry::enable_))
    {
        if (__UNLIKELY(cntr_[k][l][m] <= 0) && BASE::mask_[k][l][m])
        {
            if (__LIKELY(TDentry::inj_type_ == INJ_TYPE::BITFLIP))
            {
                BASE::data_[k][l][m] ^= BASE::mask_[k][l][m];
            }
            else if (TDentry::inj_type_ == INJ_TYPE::BIASED_S)
            {
                BASE::data_[k][l][m] |= BASE::mask_[k][l][m];
            }
            else if (TDentry::inj_type_ == INJ_TYPE::BIASED_R)
            { //  == INJ_TYPE::BIASED_R
                BASE::data_[k][l][m] &= ~BASE::mask_[k][l][m];
            }
            else
            { //== ASSIGN aka data<=mask
                BASE::data_[k][l][m] &= ~(BASE::mask_[k][l][m]);
                BASE::data_[k][l][m] |= BASE::mask_[k][l][m] & BASE::assign_value_[k][l][m];
            }
            __incr_cntr(k, l, m);
        }
    }
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
void ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::set_maskBit(unsigned bit)
{
    auto b = map_bit(bit);
    BASE::mask_[b[0]][b[1]][b[2]] |= 1 << b[3];
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
void ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::reset_mask(void)
{
    for (int k = 0; k < K; ++k)
        for (int l = 0; l < L; ++l)
            for (int m = 0; m < M; ++m)
                BASE::mask_[k][l][m] = 0;
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
void ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::set_value_bit(unsigned bit)
{
    auto b = map_bit(bit);
    BASE::assign_value_[b[0]][b[1]][b[2]] |= 1 << b[3];
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
void ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::reset_value_bit(unsigned bit)
{
    auto b = map_bit(bit);
    BASE::assign_value_[b[0]][b[1]][b[2]] &= ~(1 << b[3]);
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
void ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::reset_assign_value(void)
{
    for (int k = 0; k < K; ++k)
        for (int l = 0; l < L; ++l)
            for (int m = 0; m < M; ++m)
                BASE::assign_value_[k][l][m] = 0;
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
std::vector<bool> ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::read_data(void) const
{
    // this needs some work... generally a caller to read_data only knows the total sum of bits_,
    // thus, would only provide a buffer of such a length via pData
    // read_data has to compress the variant bitvector (either basetype extender or element slices) to a minimum
    // byte-fragmented vector... -> write helper functions that one-dimensionalize multidimensional arrays and
    // compress content based on onedimbits, dims, ...
    std::vector<bool> bitstream;
    if (BASE::onedimbits_ <= BASETYPE_BITS)
    { // array of elements, e.g., logic[1:0] x[3];
        for (int col = 0; col < K; ++col)
            for (int row = 0; row < L; ++row)
                for (int element = 0; element < M; ++element)
                    for (int bit = 0; bit < BASE::onedimbits_; ++bit)
                    {
                        vbasetype_t msk = (1 << bit);
                        bitstream.push_back((BASE::data_[col][row][element] & msk) ? true : false);
                    }
    }
    else
    { // basetype extender target, e.g., 65-bit vector in 3 WDatas
        for (int col = 0; col < K; ++col)
            for (int row = 0; row < L; ++row)
                for (int element = 0; element < M; ++element)
                {
                    if (element < (M - 1))
                    {
                        for (int bit = 0; bit < BASETYPE_BITS; ++bit)
                        {
                            vbasetype_t msk = (1 << bit);
                            bitstream.push_back((BASE::data_[col][row][element] & msk) ? true : false);
                        }
                    }
                    else
                    {
                        for (int bit = 0; bit < BASE::onedimbits_ - (M - 1) * BASETYPE_BITS; ++bit)
                        {
                            vbasetype_t msk = (1 << bit);
                            bitstream.push_back((BASE::data_[col][row][element] & msk) ? true : false);
                        }
                    }
                }
    }
    return bitstream;
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
inline void ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::inject_on_update(std::initializer_list<unsigned int> i)
{
    __inject_on_update(*(i.begin()), *(i.begin() + 1), *(i.begin() + 2));
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
inline void ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::inject_synchronous(void)
{
    for (int k = 0; k < K; ++k)
        for (int l = 0; l < L; ++l)
            for (int m = 0; m < M; ++m)
                inject(k, l, m);
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
void ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::incr_cntr(std::initializer_list<unsigned int> i)
{
    __incr_cntr(*(i.begin()), *(i.begin() + 1), *(i.begin() + 2));
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
void ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::decr_cntr(std::initializer_list<unsigned int> i)
{
    __decr_cntr(*(i.begin()), *(i.begin() + 1), *(i.begin() + 2));
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
void ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::reset_cntr(std::initializer_list<unsigned int> i)
{
    if (i.size() > 0)
    {
        __reset_cntr(*(i.begin()), *(i.begin() + 1), *(i.begin() + 2));
    }
    else
    {
        for (int k = 0; k < K; ++k)
            for (int l = 0; l < L; ++l)
                for (int m = 0; m < M; ++m)
                    __reset_cntr(k, l, m);
    }
}
template <typename vcontainer_t, typename vbasetype_t, int K, int L, int M>
std::vector<int> ThreeD_TDentry<vcontainer_t, vbasetype_t, K, L, M>::get_cntr(void)
{
    std::vector<int> c;
    for (int k = 0; k < K; ++k)
        for (int l = 0; l < L; ++l)
            for (int m = 0; m < M; ++m)
                c.push_back(cntr_[k][l][m]);
    return c;
}
} // namespace td

} // namespace vrtlfi

#endif /* __VRTLFI_TD_TARGETDICTIONARY_HPP__ */
