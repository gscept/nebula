#pragma once
//------------------------------------------------------------------------------
/**
    Game::Table

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/arrayallocator.h"
#include "util/fixedarray.h"
#include "util/string.h"
#include "util/stringatom.h"
#include "propertyid.h"

namespace MemDb
{

/// TableId contains an id reference to the database it's attached to, and the id of the table.
ID_32_TYPE(TableId);

constexpr uint16_t MAX_VALUE_TABLE_COLUMNS = 128;

/// column id
ID_16_TYPE(ColumnIndex);

struct TableCreateInfo
{
    Util::String name;
    Util::FixedArray<PropertyId> columns;
};

//------------------------------------------------------------------------------
/**
*/
class TableMask
{
public:
    TableMask() = delete;
    explicit TableMask(Util::FixedArray<PropertyId> descriptors);
    ~TableMask();

    bool const operator==(TableMask const& rhs) const;

	/// (src & mask) == mask
	static bool const CheckBits(TableMask const& src, TableMask const& mask);

private:
    __m128i* mask;
    uint8_t size;
};

//------------------------------------------------------------------------------
/**
    A table describes and holds columns, and buffers for those columns.
    
    Tables can also contain state columns, that are specific to a certain context only.
*/
struct Table
{
    using ColumnBuffer = void*;

    Util::StringAtom name;

    uint32_t numRows = 0;
    uint32_t capacity = 128;
    uint32_t grow = 128;
    // Holds freed indices to be reused in the attribute table.
    Util::Array<IndexT> freeIds;

    Util::ArrayAllocator<PropertyId, ColumnBuffer> columns;

    static constexpr Memory::HeapType HEAP_MEMORY_TYPE = Memory::HeapType::DefaultHeap;
};

//------------------------------------------------------------------------------
/**
	This class runs on the assumption that we never create a bitfield that contains more bits than necessary.
	eg, a field that is 256 bits large but is zero in left most 128 bits is forbidden.
*/
inline
TableMask::TableMask(Util::FixedArray<PropertyId> descriptors)
{
	descriptors.Sort();
	uint largestBit = (descriptors.End() - 1)->id;
	this->size = (largestBit / 128) + 1;
	this->mask = n_new_array(__m128i, this->size);
	
	for (int i = 0; i < this->size; i++)
		this->mask[i] = _mm_setzero_si128();

	alignas(16) uint64_t partialMask[2] = { 0, 0 };
	uint64_t offset = 0;
	uint64_t prevOffset = 0;
	for (PropertyId i : descriptors)
	{
		offset = i.id / 128;
		if (offset != prevOffset)
		{
			// Write value and reset partial mask
			*(this->mask + prevOffset) = _mm_set_epi64x(partialMask[0], partialMask[1]);
			prevOffset = offset;
			memset(&partialMask, 0, sizeof(partialMask));
		}
		uint32_t mod = (i.id % 128);
		partialMask[mod / 64] |= 1u << mod;
	}

	*(this->mask + offset) = _mm_set_epi64x(partialMask[0], partialMask[1]);
}

//------------------------------------------------------------------------------
/**
*/
inline
TableMask::~TableMask()
{
	n_delete_array(this->mask);
}

//------------------------------------------------------------------------------
/**
*/
inline bool const
TableMask::operator==(TableMask const& rhs) const
{
	if (this->size != rhs.size)
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
	This runs on the assumption that we never create a mask that contains more bits than necessary.
	eg, a mask that is 256 bits large but is zero in left most 128 bits is forbidden.
*/
inline bool const
TableMask::CheckBits(TableMask const& src, TableMask const& mask)
{

#ifdef NEBULA_DEBUG
	{
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

} // namespace MemDb
