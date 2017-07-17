#pragma once
//------------------------------------------------------------------------------
/**
	This class implements some static helper functions to set high and low 32-bit integers,
	as well as a function to create a complete id from two of them. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <cstdint>
#include "core/types.h"
namespace Ids
{


typedef uint64_t Id64;
typedef uint32_t Id32;
typedef uint32_t Id24;
typedef uint16_t Id16;
typedef uint8_t Id8;
static const uint64_t InvalidId64 = -1;
static const uint32_t InvalidId32 = -1;
static const uint16_t InvalidId16 = -1;
static const uint32_t InvalidId24 = -1;
static const uint8_t InvalidId8 = -1;

struct Id
{
public:

	/// set high (leftmost) 32 bits
	static void SetHigh(Id64& id, const uint32_t bits);
	/// get high (leftmost) 32 bits
	static Id32 GetHigh(const Id64 id);
	/// get high bits in integer
	static Id16 GetHigh(const Id32 bits);
	/// get big piece of 24-8 bit integer
	static Id24 GetBig(const Id32 bits);
	/// get tiny piece of 24-8 bit integer
	static Id8 GetTiny(const Id32 bits);
	/// set low (rightmost) 32 bits
	static void SetLow(Id64& id, const Id32 bits);
	/// get low (rightmost) 32 bits
	static Id32 GetLow(const Id64 id);
	/// get low bits in integer
	static Id16 GetLow(const Id32 bits);
	/// create new Id using both high and low bits
	static Id64 MakeId64(const Id32 low, const Id32 high);
	/// set high bits in half Id, which returns an integer
	static Id32 MakeId32(const Id16 high, const Id16 low);
	/// set 24-8 bits in integer
	static Id32 MakeId24_8(const Id24 big, const Id8 tiny);

};

//------------------------------------------------------------------------------
/**
*/
inline void
Id::SetHigh(Id64& id, const Id32 bits)
{
	id = (((uint64_t)bits) << 32) & 0xFFFFFFFF00000000;
}

//------------------------------------------------------------------------------
/**
*/
inline Id32
Id::GetHigh(const Id64 id)
{
	uint32_t bits = (uint32_t)((id >> 32) & 0x00000000FFFFFFFF);
	return bits;
}

//------------------------------------------------------------------------------
/**
*/
inline Id16
Id::GetHigh(const Id32 bits)
{
	uint16_t ret = (uint16_t)((bits >> 16) & 0x0000FFFF);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline Id32
Id::GetBig(const Id32 bits)
{
	return (bits & 0xFFFFFF00) >> 8;
}

//------------------------------------------------------------------------------
/**
*/
inline Id8
Id::GetTiny(const Id32 bits)
{
	uint8_t ret = (bits & 0x000000FF);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Id::SetLow(Id64& id, const Id32 bits)
{
	id = ((uint64_t)bits) & 0x00000000FFFFFFFF;
}

//------------------------------------------------------------------------------
/**
*/
inline Id32
Id::GetLow(const Id64 id)
{
	uint32_t bits = (uint32_t)((id) & 0x00000000FFFFFFFF);
	return bits;
}

//------------------------------------------------------------------------------
/**
*/
inline Id16
Id::GetLow(const Id32 bits)
{
	int16_t ret = (uint16_t)((bits) & 0x0000FFFF);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline Ids::Id64
Id::MakeId64(const Id32 high, const Id32 low)
{
	Ids::Id64 ret;
	ret = (((uint64_t)low) & 0x00000000FFFFFFFF) + ((((uint64_t)high) << 32) & 0xFFFFFFFF00000000);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline Id32
Id::MakeId32(const Id16 high, const Id16 low)
{
	uint32_t ret = (((uint32_t)low) & 0x0000FFFF) + ((((uint32_t)high) << 16) & 0xFFFF0000);
	return ret;
}


//------------------------------------------------------------------------------
/**
*/
inline Id32
Id::MakeId24_8(const Id24 big, const Id8 tiny)
{
	uint32_t ret = (((uint32_t)tiny) & 0x000000FF) + ((big << 8) & 0xFFFFFF00);
	return ret;
}

} // namespace Ids