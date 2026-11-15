#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::BitArray

    Implements a growable bit array (similar to std::vector<bool>).

    Unlike BitField, which is sized at compile time, this grows when a bit outside it is set, which is what
    makes it usable for a flag per item when the number of items is not known in advance, for example a flag
    per entity or per instance of a runtime structure.

    The storage is one 64 bit word per 64 bits, so a whole section can be skipped with a single word check
    before any bit is looked at.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/array.h"

namespace Util
{
//------------------------------------------------------------------------------
/**
*/
class BitArray
{
public:
    /// constructor
    BitArray();
    /// destructor
    ~BitArray();

    /// resize to a number of bits. Growing keeps the bits which are already set, shrinking is not supported
    void Resize(uint64_t numBits);
    /// make a bit addressable, growing the array when it is outside
    void EnsureBit(uint64_t bitIndex);

    /// set a bit by index, growing the array when the index is outside it
    void SetBit(const uint64_t bitIndex);
    /// clear a bit by index. Does nothing when the index is outside the array
    void ClearBit(const uint64_t bitIndex);
    /// check a bit by index. False when the index is outside the array
    bool IsSet(const uint64_t bitIndex) const;
    /// move a bit from one index to another. The source is cleared and the target gets the value
    void MoveBit(const uint64_t from, const uint64_t to);

    /// true when no bit is set
    bool IsNull() const;
    /// true when a whole 64 bit section is empty
    bool SectionIsNull(const uint64_t section) const;
    /// number of 64 bit sections which are allocated
    SizeT NumSections() const;
    /// the index of the first set bit in a section, or InvalidIndex when the section is empty
    IndexT FirstSetBitInSection(const uint64_t section) const;

    /// clear all bits. The allocated words are kept
    void Clear();
    /// clear all bits and release the storage
    void Reset();

private:
    Util::Array<uint64_t> words;
};

//------------------------------------------------------------------------------
/**
*/
inline
BitArray::BitArray()
{
}

//------------------------------------------------------------------------------
/**
*/
inline
BitArray::~BitArray()
{
}

//------------------------------------------------------------------------------
/**
*/
inline void
BitArray::Resize(uint64_t numBits)
{
    SizeT const numWords = (SizeT)((numBits + 63) / 64);
    if (numWords > this->words.Size())
    {
        this->words.Resize(numWords, 0);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
BitArray::EnsureBit(const uint64_t bitIndex)
{
    this->Resize(bitIndex + 1);
}

//------------------------------------------------------------------------------
/**
*/
inline void
BitArray::SetBit(const uint64_t bitIndex)
{
    this->Resize(bitIndex + 1);
    this->words[bitIndex / 64] |= (1ULL << (bitIndex % 64));
}

//------------------------------------------------------------------------------
/**
*/
inline void
BitArray::ClearBit(const uint64_t bitIndex)
{
    if (bitIndex / 64 >= (uint64_t)this->words.Size())
    {
        return;
    }
    this->words[bitIndex / 64] &= ~(1ULL << (bitIndex % 64));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
BitArray::IsSet(const uint64_t bitIndex) const
{
    if (bitIndex / 64 >= (uint64_t)this->words.Size())
    {
        return false;
    }
    return (this->words[bitIndex / 64] >> (bitIndex % 64)) & 1ULL;
}

//------------------------------------------------------------------------------
/**
*/
inline void
BitArray::MoveBit(const uint64_t from, const uint64_t to)
{
    bool const set = this->IsSet(from);
    this->ClearBit(from);
    if (set)
    {
        this->SetBit(to);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline bool
BitArray::IsNull() const
{
    for (IndexT i = 0; i < this->words.Size(); i++)
    {
        if (this->words[i] != 0)
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
BitArray::SectionIsNull(const uint64_t section) const
{
    if (section >= (uint64_t)this->words.Size())
    {
        return true;
    }
    return this->words[section] == 0;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
BitArray::NumSections() const
{
    return this->words.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
BitArray::FirstSetBitInSection(const uint64_t section) const
{
    if (section >= (uint64_t)this->words.Size())
    {
        return InvalidIndex;
    }
    uint64_t const word = this->words[section];
    if (word == 0)
    {
        return InvalidIndex;
    }
    IndexT bit = 0;
    uint64_t remaining = word;
    while ((remaining & 1ULL) == 0)
    {
        remaining >>= 1;
        bit++;
    }
    return (IndexT)(section * 64 + (uint64_t)bit);
}

//------------------------------------------------------------------------------
/**
*/
inline void
BitArray::Clear()
{
    for (IndexT i = 0; i < this->words.Size(); i++)
    {
        this->words[i] = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
BitArray::Reset()
{
    this->words.Clear();
}

} // namespace Util
//------------------------------------------------------------------------------
