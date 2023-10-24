#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::BitField
    
    Implements large bit field.
    
    The bit field occupies as little memory as possible, based on how many bits
    are required per the template parameter.

    Will occupy 8, 16, 32 or 64 bits, or a multiple of 64 bits for larger
    fields.
    
    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{
template <unsigned int NUMBITS> class BitField
{
    static_assert(NUMBITS > 0);

public:
    /// constructor
    BitField();
    /// constructs a bitfield based on multiple values
    constexpr BitField(std::initializer_list<unsigned int> list);
    /// copy constructor
    BitField(const BitField<NUMBITS>& rhs) = default;

    /// equality operator
    constexpr bool operator==(const BitField<NUMBITS>& rhs) const;
    /// inequality operator
    constexpr bool operator!=(const BitField<NUMBITS>& rhs) const;

    /// Check if single bit is set
    constexpr bool IsSet(const uint64_t bitIndex) const;
    /// Check if single bit is set
    template <uint64_t bitIndex> constexpr bool IsSet() const;
    /// clear content
    void Clear();
    /// return true if all bits are 0
    bool IsNull() const;
    /// set a bit by index
    constexpr void SetBit(const uint64_t bitIndex);
    /// set a bit by index. Multiplies the bit with cond before OR-ing which means it won't set the bit if mul is 0, which makes it branchless.
    constexpr void SetBitIf(const uint64_t bitIndex, uint64_t cond);
    /// set a bit by index
    template <uint64_t bitIndex> constexpr void SetBit();
    /// clear a bit by index
    void ClearBit(const uint64_t bitIndex);
    
    /// Check if a section of the bitfield is set.
    /// The section size depends on the size of the bit field.
    /// If the bitfield is 8, 16 or 32 bits large, the section size is
    /// the same size as the bitfield, thus there's only one section.
    /// If the bitfield is 64 bits or larger, the section size is 64 bits per section.
    bool SectionIsNull(uint64_t section) const;

    /// set bitfield to OR combination
    static constexpr BitField<NUMBITS> Or(const BitField<NUMBITS>& b0, const BitField<NUMBITS>& b1);
    /// set bitfield to AND combination
    static constexpr BitField<NUMBITS> And(const BitField<NUMBITS>& b0, const BitField<NUMBITS>& b1);

private:
    static constexpr uint64_t BASE = NUMBITS > 32 ? 64 : NUMBITS > 16 ? 32 : NUMBITS > 8 ? 16 : 8;
    static const int size = ((NUMBITS + BASE - 1) / BASE);

    // Template magic to automatically use the smallest type possible
    template <size_t S> struct BitType
    {
        using T = uint8_t;
    };
    template <> struct BitType<16>
    {
        using T = uint16_t;
    };
    template <> struct BitType<32>
    {
        using T = uint32_t;
    };
    template <> struct BitType<64>
    {
        using T = uint64_t;
    };

    using TYPE = typename BitType<BASE>::T;

    TYPE bits[size];
};

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS> BitField<NUMBITS>::BitField()
{
    IndexT i;
    for (i = 0; i < size; i++)
    {
        this->bits[i] = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS> constexpr BitField<NUMBITS>::BitField(std::initializer_list<unsigned int> list)
{
    for (IndexT i = 0; i < size; i++)
    {
        this->bits[i] = 0;
    }

    for (auto bit : list)
    {
        this->SetBit(bit);
    }
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
constexpr bool
BitField<NUMBITS>::operator==(const BitField<NUMBITS>& rhs) const
{
    for (IndexT i = 0; i < size; i++)
    {
        if (this->bits[i] != rhs.bits[i])
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
constexpr bool
BitField<NUMBITS>::operator!=(const BitField<NUMBITS>& rhs) const
{
    return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
constexpr bool
BitField<NUMBITS>::IsSet(const uint64_t bitIndex) const
{
    n_assert(bitIndex < NUMBITS);
    const TYPE i = (1ull << (bitIndex % BASE));
    const TYPE index = bitIndex / BASE;
    return (this->bits[index] & i) == i;
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
template <uint64_t bitIndex>
constexpr bool
BitField<NUMBITS>::IsSet() const
{
    static_assert(bitIndex < NUMBITS);
    constexpr TYPE i = (1ull << (bitIndex % BASE));
    constexpr TYPE index = bitIndex / BASE;
    return (this->bits[index] & i) == i;
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
void
BitField<NUMBITS>::Clear()
{
    IndexT i;
    for (i = 0; i < size; i++)
    {
        this->bits[i] = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
bool
BitField<NUMBITS>::IsNull() const
{
    IndexT i;
    for (i = 0; i < size; i++)
    {
        if (this->bits[i] != 0)
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
constexpr void
BitField<NUMBITS>::SetBit(const uint64_t i)
{
    n_assert(i < NUMBITS);
    const TYPE index = i / BASE;
    const TYPE bit = (1ull << (i % BASE));
    this->bits[index] |= bit;
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
constexpr void
BitField<NUMBITS>::SetBitIf(const uint64_t i, uint64_t cond)
{
    n_assert(i < NUMBITS);
    const TYPE index = i / BASE;
    const TYPE bit = (1ull << (i % BASE));
    this->bits[index] |= bit * cond;
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
template <uint64_t i>
constexpr void
BitField<NUMBITS>::SetBit()
{
    static_assert(i < NUMBITS);
    constexpr TYPE index = i / BASE;
    constexpr TYPE bit = (1ull << (i % BASE));
    this->bits[index] |= bit;
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
void
BitField<NUMBITS>::ClearBit(const uint64_t i)
{
    n_assert(i < NUMBITS);
    const TYPE index = i / BASE;
    const TYPE bit = ~(1ull << (i % BASE));
    this->bits[index] &= bit;
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
bool
BitField<NUMBITS>::SectionIsNull(uint64_t section) const
{
    n_assert(section < this->size);
    return this->bits[section] == 0;
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
constexpr BitField<NUMBITS>
BitField<NUMBITS>::Or(const BitField<NUMBITS>& b0, const BitField<NUMBITS>& b1)
{
    BitField<NUMBITS> res;
    for (IndexT i = 0; i < size; i++)
    {
        res.bits[i] = b0.bits[i] | b1.bits[i];
    }
    return res;
}

//------------------------------------------------------------------------------
/**
*/
template <unsigned int NUMBITS>
constexpr BitField<NUMBITS>
BitField<NUMBITS>::And(const BitField<NUMBITS>& b0, const BitField<NUMBITS>& b1)
{
    BitField<NUMBITS> res;
    for (IndexT i = 0; i < size; i++)
    {
        res.bits[i] = b0.bits[i] & b1.bits[i];
    }
    return res;
}

} // namespace Util
//------------------------------------------------------------------------------
