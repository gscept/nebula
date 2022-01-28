#pragma once
//------------------------------------------------------------------------------
/**
    @class  MemDb::TableSignature

    Basically a bitfield with packed ComponentIds.

    Can be used to identify tables, or be used as a mask to query the database for tables that
    contain a certain set of components.

    @note   This class runs on the assumption that we never create a bitfield that contains more bits than necessary.
    i.e, a field that is 256 bits large but is zero in left most 128 bits is forbidden.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/fixedarray.h"
#include "propertyid.h"

namespace MemDb
{

class TableSignature
{
public:
    /// default constructor.
    TableSignature() : mask(nullptr), size(0) {};
    /// copy constructor
    TableSignature(TableSignature const& rhs);
    /// construct from fixed array of component ids
    TableSignature(Util::FixedArray<ComponentId> const& descriptors);
    /// construct from component id initializer list, for convenience
    TableSignature(std::initializer_list<ComponentId> descriptors);
    /// construct from component id pointer array
    TableSignature(ComponentId const* descriptors, SizeT num);
    /// destructor
    ~TableSignature();
    /// assignment operator
    TableSignature& operator=(TableSignature const& rhs);
    /// equality operator
    bool const operator==(TableSignature const& rhs) const;
    /// check if signature is valid
    bool const IsValid() const { return size > 0; }
    /// check if a single bit is set
    bool const IsSet(ComponentId component) const;
    /// flip a bit.
    void FlipBit(ComponentId component);

    /// (src & mask) == mask
    static bool const CheckBits(TableSignature const& src, TableSignature const& mask);
    /// check if src has any of the bits in mask set ((src & mask) == 0)
    static bool const HasAny(TableSignature const& src, TableSignature const& mask);

protected:
    /// create bitfield from fixed array.
    void Setup(ComponentId const* descriptors, SizeT num);

    /// large bit field, using SSE registers
    __m128i* mask;
    /// number of SSE regs allocated
    uint8_t size;
};

//------------------------------------------------------------------------------
/**
*/
inline
TableSignature::TableSignature(Util::FixedArray<ComponentId> const& descriptors)
{
    this->Setup(descriptors.Begin(), descriptors.Size());
}

//------------------------------------------------------------------------------
/**
*/
inline
TableSignature::TableSignature(std::initializer_list<ComponentId> descriptors)
{
    this->Setup(descriptors.begin(), (SizeT)descriptors.size());
}

//------------------------------------------------------------------------------
/**
*/
inline
TableSignature::TableSignature(ComponentId const* descriptors, SizeT num)
{
    this->Setup(descriptors, num);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
TableSignature::Setup(ComponentId const* componentBuffer, SizeT num)
{
    if (num > 0)
    {
        Util::Array<ComponentId> descriptors(componentBuffer, num);
        descriptors.Sort();
        ComponentId const last = *(descriptors.End() - 1);
        n_assert(last != ComponentId::Invalid());
        uint const largestBit = last.id;
        this->size = (largestBit / 128) + 1;
        this->mask = (__m128i*)Memory::Alloc(Memory::HeapType::ObjectHeap, this->size * 16);

        for (int i = 0; i < this->size; i++)
            this->mask[i] = _mm_setzero_si128();

        alignas(16) uint64_t partialMask[2] = { 0, 0 };
        // offsets start at first values offset
        uint64_t offset = descriptors[0].id / 128;
        uint64_t prevOffset = offset;
        for (ComponentId i : descriptors)
        {
            n_assert(i != ComponentId::Invalid());
            offset = i.id / 128;
            if (offset != prevOffset)
            {
                // Write value and reset partial mask
                *(this->mask + prevOffset) = _mm_set_epi64x(partialMask[0], partialMask[1]);
                prevOffset = offset;
                memset(&partialMask, 0, sizeof(partialMask));
            }
            uint32_t mod = (i.id % 128);
            partialMask[mod / 64] |= 1ull << mod;
        }

        *(this->mask + offset) = _mm_set_epi64x(partialMask[0], partialMask[1]);
    }
    else
    {
        this->mask = nullptr;
        this->size = 0;
    }
}


//------------------------------------------------------------------------------
/**
*/
inline
TableSignature::TableSignature(TableSignature const& rhs) :
    size(rhs.size)
{
    if (size > 0)
    {
        this->mask = (__m128i*)Memory::Alloc(Memory::HeapType::ObjectHeap, this->size * 16);
        for (int i = 0; i < this->size; i++)
            this->mask[i] = _mm_load_si128(rhs.mask + i);
    }
    else
    {
        this->mask = nullptr;
    }
};

//------------------------------------------------------------------------------
/**
*/
inline
TableSignature::~TableSignature()
{
    if (this->mask != nullptr)
        Memory::Free(Memory::HeapType::ObjectHeap, this->mask);
}

//------------------------------------------------------------------------------
/**
*/
inline bool const
TableSignature::operator==(TableSignature const& rhs) const
{
    if (this->size != rhs.size || this->size == 0 || rhs.size == 0)
    {
        return false;
    }

    for (int i = 0; i < this->size; i++)
    {
        __m128i temp = _mm_cmpeq_epi32(this->mask[i], rhs.mask[i]);
        if ((_mm_movemask_epi8(temp) != 0xFFFF))
            return false;
    };

    return true;
}

//------------------------------------------------------------------------------
/**
*/
inline bool const
TableSignature::IsSet(ComponentId component) const
{
    int offset = component.id / 128;
    if (offset < this->size)
    {
        alignas(16) uint64_t partialMask[2] = { 0, 0 };
        uint64_t const bit = component.id % 128;
        partialMask[bit / 64] |= 1ull << bit;
        __m128i temp = _mm_set_epi64x(partialMask[0], partialMask[1]);
        int const isSet = !_mm_testz_si128(temp, this->mask[offset]);
        return isSet;
    }

    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TableSignature::FlipBit(ComponentId component)
{
    int offset = component.id / 128;
    n_assert(offset < this->size); // currently, we can't flip a bit that is outside the signatures size.
    alignas(16) uint64_t partialMask[2] = { 0, 0 };
    uint64_t bit = component.id % 128;
    partialMask[bit / 64] |= 1ull << bit;
    __m128i temp = _mm_set_epi64x(partialMask[0], partialMask[1]);
    this->mask[offset] = _mm_and_si128(this->mask[offset], temp);
}

//------------------------------------------------------------------------------
/**
*/
inline TableSignature&
TableSignature::operator=(TableSignature const& rhs)
{
    this->size = rhs.size;
    if (this->size > 0)
    {
        this->mask = (__m128i*)Memory::Alloc(Memory::HeapType::ObjectHeap, this->size * 16);
        for (int i = 0; i < this->size; i++)
            this->mask[i] = _mm_load_si128(rhs.mask + i);
    }
    else
    {
        this->mask = nullptr;
    }
    return *this;
}

//------------------------------------------------------------------------------
/**
    This runs on the assumption that we never create a mask that contains more bits than necessary.
    i.e, a mask that is 256 bits large but is zero in left most 128 bits is forbidden.
*/
inline bool const
TableSignature::CheckBits(TableSignature const& src, TableSignature const& mask)
{
    if (src.size == 0 || mask.size == 0)
        return false;

#ifdef NEBULA_DEBUG
    {
        // validation
        __m128i cmp = _mm_cmpeq_epi32(mask.mask[mask.size - 1], _mm_setzero_si128());
        n_assert(_mm_movemask_epi8(cmp) != 0xFFFF);
    }
#endif

    if (mask.size > src.size)
    {
        return false;
    }

    for (int i = 0; i < mask.size; i++)
    {
        __m128i temp = _mm_and_si128(src.mask[i], mask.mask[i]);
        __m128i cmp = _mm_cmpeq_epi32(temp, mask.mask[i]);
        if ((_mm_movemask_epi8(cmp) != 0xFFFF))
            return false;
    };

    return true;
}

//------------------------------------------------------------------------------
/**
    This runs on the assumption that we never create a mask that contains more bits than necessary.
    i.e, a mask that is 256 bits large but is zero in left most 128 bits is forbidden.
*/
inline bool const
TableSignature::HasAny(TableSignature const& src, TableSignature const& mask)
{
    if (src.size == 0 || mask.size == 0)
        return false;

#ifdef NEBULA_DEBUG
    {
        // validation
        __m128i cmp = _mm_cmpeq_epi32(mask.mask[mask.size - 1], _mm_setzero_si128());
        n_assert(_mm_movemask_epi8(cmp) != 0xFFFF);
    }
#endif

    const int size = Math::min(mask.size, src.size);

    for (int i = 0; i < size; i++)
    {
        __m128i temp = _mm_and_si128(src.mask[i], mask.mask[i]);
        __m128i cmp = _mm_cmpeq_epi32(temp, _mm_setzero_si128());
        if ((_mm_movemask_epi8(cmp) == 0xFFFF))
            return false;
    };

    return true;
}

} // namespace MemDb
