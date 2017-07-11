#pragma once
//------------------------------------------------------------------------------
/**
	An Id is a 64-bit integer which is to be used as handles to internal systems,
	like OpenGL. 

	This class implements some static helper functions to set high and low 32-bit integers,
	as well as a function to create a complete id from two of them. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <cstdint>
#include "core/types.h"
namespace Core
{
struct Id
{
public:

	/// constructor
	Id();

	/// set high (leftmost) 32 bits
	static void SetHigh(Id& id, const uint32_t bits);
	/// get high (leftmost) 32 bits
	static uint32_t GetHigh(const Id& id);
	/// get high bits in integer
	static uint16_t GetHigh(const uint32_t bits);
	/// get big piece of 24-8 bit integer
	static uint32_t GetBig(const uint32_t bits);
	/// get tiny piece of 24-8 bit integer
	static char GetTiny(const uint32_t bits);
	/// set low (rightmost) 32 bits
	static void SetLow(Id& id, const uint32_t bits);
	/// get low (rightmost) 32 bits
	static uint32_t GetLow(const Id& id);
	/// get low bits in integer
	static uint16_t GetLow(const uint32_t bits);
	/// create new Id using both high and low bits
	static Id MakeId(const uint32_t low, const uint32_t high);
	/// set high bits in half Id, which returns an integer
	static uint32_t MakeHalfId(const uint16_t high, const uint16_t low);
	/// set 24-8 bits in integer
	static uint32_t MakeBigTiny(const uint32_t big, const uchar tiny);

	uint64_t id;
	static const uint64_t InvalidId = -1;
};

//------------------------------------------------------------------------------
/**
*/
inline 
Id::Id() :
	id(InvalidId)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
Id::SetHigh(Id& id, const uint32_t bits)
{
	id.id = (((uint64_t)bits) << 32) & 0xFFFFFFFF00000000;
}

//------------------------------------------------------------------------------
/**
*/
inline uint32_t
Id::GetHigh(const Id& id)
{
	uint32_t bits = (uint32_t)((id.id >> 32) & 0x00000000FFFFFFFF);
	return bits;
}

//------------------------------------------------------------------------------
/**
*/
inline uint16_t
Id::GetHigh(const uint32_t bits)
{
	uint16_t ret = (uint16_t)((bits >> 16) & 0x0000FFFF);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline uint32_t
Id::GetBig(const uint32_t bits)
{
	return (bits & 0xFFFFFF00) >> 8;
}

//------------------------------------------------------------------------------
/**
*/
inline char
Id::GetTiny(const uint32_t bits)
{
	char ret = (bits & 0x000000FF);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Id::SetLow(Id& id, const uint32_t bits)
{
	id.id = ((uint64_t)bits) & 0x00000000FFFFFFFF;
}

//------------------------------------------------------------------------------
/**
*/
inline uint32_t
Id::GetLow(const Id& id)
{
	uint32_t bits = (uint32_t)((id.id) & 0x00000000FFFFFFFF);
	return bits;
}

//------------------------------------------------------------------------------
/**
*/
inline uint16_t
Id::GetLow(const uint32_t bits)
{
	int16_t ret = (uint16_t)((bits) & 0x0000FFFF);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline Core::Id
Id::MakeId(const uint32_t high, const uint32_t low)
{
	Core::Id ret;
	ret.id = (((uint64_t)low) & 0x00000000FFFFFFFF) + ((((uint64_t)high) << 32) & 0xFFFFFFFF00000000);
	return ret;
}


//------------------------------------------------------------------------------
/**
*/
inline uint32_t
Id::MakeHalfId(const uint16_t high, const uint16_t low)
{
	uint32_t ret = (((uint32_t)low) & 0x0000FFFF) + ((((uint32_t)high) << 16) & 0xFFFF0000);
	return ret;
}


//------------------------------------------------------------------------------
/**
*/
inline uint32_t
Id::MakeBigTiny(const uint32_t big, const uchar tiny)
{
	uint32_t ret = (((uint32_t)tiny) & 0x000000FF) + ((big << 8) & 0xFFFFFF00);
	return ret;
}

} // namespace Core