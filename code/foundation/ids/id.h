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
static const uint64_t InvalidId64 = 0xFFFFFFFFFFFFFFFF;
static const uint32_t InvalidId32 = 0xFFFFFFFF;
static const uint32_t InvalidId24 = 0x00FFFFFF;
static const uint16_t InvalidId16 = 0xFFFF;
static const uint8_t InvalidId8 = 0xFF;

#define ID_64_TYPE(x) struct x { Ids::Id64 id; constexpr x() : id(0) {}; constexpr x(const Ids::Id64 id) : id(id) {}; constexpr operator Ids::Id64() const  { return id; } static constexpr x Invalid() { return Ids::InvalidId64; } constexpr IndexT HashCode() const { return (IndexT)(id & 0x00000000FFFFFFFF); } };
#define ID_32_TYPE(x) struct x { Ids::Id32 id; constexpr x() : id(0) {}; constexpr x(const Ids::Id32 id) : id(id) {}; constexpr operator Ids::Id32() const  { return id; } static constexpr x Invalid() { return Ids::InvalidId32; } constexpr IndexT HashCode() const { return id; } };
#define ID_24_TYPE(x) struct x { Ids::Id24 id : 24;  constexpr x() : id(0) {}; constexpr x(const Ids::Id32 id) : id(id) {}; constexpr operator Ids::Id24() const  { return id; } static constexpr x Invalid() { return Ids::InvalidId24; } constexpr IndexT HashCode() const { return (IndexT)(id); } };
#define ID_16_TYPE(x) struct x { Ids::Id16 id; constexpr x() : id(0) {}; constexpr x(const Ids::Id16 id) : id(id) {}; constexpr operator Ids::Id16() const  { return id; } static constexpr x Invalid() { return Ids::InvalidId16; } constexpr IndexT HashCode() const { return (IndexT)(id); } };
#define ID_8_TYPE(x) struct x { Ids::Id8 id;  constexpr x() : id(0) {}; constexpr x(const Ids::Id8 id) : id(id) {}; constexpr operator Ids::Id8() const  { return id; } static constexpr x Invalid() { return Ids::InvalidId8; } constexpr IndexT HashCode() const { return (IndexT)(id); } };


#define ID_32_24_8_TYPE(x) struct x { \
	Ids::Id32 id32 : 32;\
	Ids::Id24 id24 : 24;\
	Ids::Id8 id8: 8;\
	constexpr x() : id32(0), id24(0), id8(0) {};\
	constexpr x(const Ids::Id64 id) : id32(Ids::Id::GetHigh(id)), id24(Ids::Id::GetBig(Ids::Id::GetLow(id))), id8(Ids::Id::GetTiny(Ids::Id::GetLow(id))) {};\
	constexpr operator Ids::Id64() const { return Ids::Id::MakeId32_24_8(id32, id24, id8); }\
	static constexpr x Invalid() { return Ids::Id::MakeId32_24_8(Ids::InvalidId32, Ids::InvalidId24, Ids::InvalidId8); }\
	constexpr IndexT HashCode() const { return (IndexT)(id24 & 0xFFFFFF00 | id8); }\
	constexpr Ids::Id64 HashCode64() const { return Ids::Id::MakeId32_24_8(id32, id24, id8); }\
	};

#define ID_24_8_24_8_TYPE(x) struct x { \
	Ids::Id24 id24_0 : 24;\
	Ids::Id8 id8_0 : 8;\
	Ids::Id24 id24_1 : 24;\
	Ids::Id8 id8_1 : 8;\
	constexpr x() : id24_0(0), id8_0(0), id24_1(0), id8_1(0) {};\
	constexpr x(const Ids::Id64 id) : id24_0(Ids::Id::GetBig(Ids::Id::GetHigh(id))), id8_0(Ids::Id::GetTiny(Ids::Id::GetHigh(id))), id24_1(Ids::Id::GetBig(Ids::Id::GetLow(id))), id8_1(Ids::Id::GetTiny(Ids::Id::GetLow(id))) {};\
	constexpr operator Ids::Id64() const { return Ids::Id::MakeId24_8_24_8(id24_0, id8_0, id24_1, id8_1); }\
	static constexpr x Invalid() { return Ids::Id::MakeId24_8_24_8(Ids::InvalidId24, Ids::InvalidId8, Ids::InvalidId24, Ids::InvalidId8); }\
	constexpr IndexT HashCode() const { return (IndexT)(id24_0 & 0xFFFFFF00 | id8_0); }\
	constexpr Ids::Id64 HashCode64() const { return Ids::Id::MakeId24_8_24_8(id24_0, id8_0, id24_1, id8_1); }\
	};

#define ID_32_32_TYPE(x) struct x { \
	Ids::Id32 id32_0; \
	Ids::Id32 id32_1; \
	constexpr x() : id32_0(0), id32_1(0) {} \
	constexpr x(const Ids::Id64 id) : id32_0(Ids::Id::GetHigh(id)), id32_1(Ids::Id::GetLow(id)) {};\
	constexpr operator Ids::Id64() const  { return Ids::Id::MakeId64(id32_0, id32_1); }\
	static constexpr x Invalid() { return Ids::Id::MakeId64(Ids::InvalidId32, Ids::InvalidId32); }\
	constexpr IndexT HashCode() const { return (IndexT)(id32_1); }\
	};

#define ID_24_8_TYPE(x) struct x { \
	Ids::Id32 id24 : 24; \
	Ids::Id8 id8: 8; \
	constexpr x() : id24(0), id8(0) {}  \
	constexpr x(const Ids::Id32 id) : id24(Ids::Id::GetBig(id)), id8(Ids::Id::GetTiny(id)) {};\
	constexpr operator Ids::Id32() const { return Ids::Id::MakeId24_8(id24, id8); }\
	static constexpr x Invalid() { return Ids::Id::MakeId24_8(Ids::InvalidId24, Ids::InvalidId8); }\
	constexpr IndexT HashCode() const { return (IndexT)(id24 & 0xFFFFFF00 | id8); }\
	};

struct Id
{
	/// set high (leftmost) 32 bits
	static void SetHigh(Id64& id, const Id32 bits);
	/// get high (leftmost) 32 bits
	static constexpr Id32 GetHigh(const Id64 id);
	/// get high bits in integer
	static constexpr Id16 GetHigh(const Id32 bits);
	/// get big piece of 24-8 bit integer
	static constexpr Id24 GetBig(const Id32 bits);
	/// get tiny piece of 24-8 bit integer
	static constexpr Id8 GetTiny(const Id32 bits);
	/// set low (rightmost) 32 bits
	static void SetLow(Id64& id, const Id32 bits);
	/// get low (rightmost) 32 bits
	static constexpr Id32 GetLow(const Id64 id);
	/// get low bits in integer
	static constexpr Id16 GetLow(const Id32 bits);
	/// create new Id using both high and low bits
	static constexpr Id64 MakeId64(const Id32 low, const Id32 high);
	/// set high bits in half Id, which returns an integer
	static constexpr Id32 MakeId32(const Id16 high, const Id16 low);
	/// set 24-8 bits in integer
	static constexpr Id32 MakeId24_8(const Id24 big, const Id8 tiny);
	/// set 32-24-8 bits 64 bit integer
	static constexpr Id64 MakeId32_24_8(const Id32 upper, const Id24 big, const Id8 tiny);
	/// set 24-8-24-8 bits 64 bit integer
	static constexpr Id64 MakeId24_8_24_8(const Id24 big0, const Id8 tiny0, const Id24 big1, const Id8 tiny1);
	/// split 64 bit integer into 2 32 0bit integers
	static void Split64(Id64 split, Id32& upper, Id32& lower);
	/// split 32 bit integer into 2 16 bit integers
	static void Split32(Id32 split, Id16& upper, Id16& lower);
	/// split 32 bit integer into 24-8 bit integer
	static void Split24_8(Id32 split, Id24& big, Id8& tiny);
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
inline constexpr Id32
Id::GetHigh(const Id64 id)
{
	return (uint32_t)((id >> 32) & 0x00000000FFFFFFFF);
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Id16
Id::GetHigh(const Id32 bits)
{
	return (uint16_t)((bits >> 16) & 0x0000FFFF);
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Id32
Id::GetBig(const Id32 bits)
{
	return (bits & 0xFFFFFF00) >> 8;
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Id8
Id::GetTiny(const Id32 bits)
{
	return (bits & 0x000000FF);
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
inline constexpr Id32
Id::GetLow(const Id64 id)
{
	return (uint32_t)((id) & 0x00000000FFFFFFFF);
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Id16
Id::GetLow(const Id32 bits)
{
	return (uint16_t)((bits) & 0x0000FFFF);
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Ids::Id64
Id::MakeId64(const Id32 high, const Id32 low)
{
	return (((uint64_t)low) & 0x00000000FFFFFFFF) + ((((uint64_t)high) << 32) & 0xFFFFFFFF00000000);
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Id32
Id::MakeId32(const Id16 high, const Id16 low)
{
	return (((uint32_t)low) & 0x0000FFFF) + ((((uint32_t)high) << 16) & 0xFFFF0000);
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Id32
Id::MakeId24_8(const Id24 big, const Id8 tiny)
{
	return (((uint32_t)tiny) & 0x000000FF) + ((big << 8) & 0xFFFFFF00);
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Id64
Id::MakeId32_24_8(const Id32 upper, const Id24 big, const Id8 tiny)
{
	return (((uint64_t)upper << 32) & 0xFFFFFFFF00000000) + (((uint64_t)tiny) & 0x00000000000000FF) + (((uint64_t)big << 8) & 0x00000000FFFFFF00);
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Id64
Id::MakeId24_8_24_8(const Id24 big0, const Id8 tiny0, const Id24 big1, const Id8 tiny1)
{
	return (((uint64_t)big0 << 32) & 0xFFFFFF0000000000) + (((uint64_t)tiny0 << 32) & 0x000000FF00000000) + (((uint64_t)big1 << 8) & 0x00000000FFFFFF00) + (((uint64_t)tiny1) & 0x00000000000000FF);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Id::Split64(Id64 split, Id32& upper, Id32& lower)
{
	upper = GetHigh(split);
	lower = GetLow(split);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Id::Split32(Id32 split, Id16& upper, Id16& lower)
{
	upper = GetHigh(split);
	lower = GetLow(split);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Id::Split24_8(Id32 split, Id24& big, Id8& tiny)
{
	big = GetBig(split);
	tiny = GetTiny(split);
}

} // namespace Ids