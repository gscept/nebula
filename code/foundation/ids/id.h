#pragma once
//------------------------------------------------------------------------------
/**
    @class Ids::Id

    This class implements some static helper functions to set high and low 32-bit integers,
    as well as a function to create a complete id from two of them. 
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <cstdint>
#include "core/types.h"

#define ID_32_TYPE(x) struct x { \
    Ids::Id32 id; \
    constexpr x() : id(Ids::InvalidId32) {}; \
    constexpr x(const Ids::Id32 id) : id(id) {}; \
    constexpr explicit operator Ids::Id32() const { return id; } \
    static constexpr x Invalid() { return Ids::InvalidId32; } \
    constexpr uint32_t HashCode() const { return id; } \
    const bool operator==(const x& rhs) const { return id == rhs.id; } \
    const bool operator!=(const x& rhs) const { return id != rhs.id; } \
    const bool operator<(const x& rhs) const { return HashCode() < rhs.HashCode(); } \
    const bool operator>(const x& rhs) const { return HashCode() > rhs.HashCode(); } \
    template <typename T> constexpr T As() const { static_assert(sizeof(T) == sizeof(x), "Can only convert between ID types of equal size"); T ret; memcpy((void*)&ret, this, sizeof(T)); return ret; }; \
    }; \
    static constexpr x Invalid##x = Ids::InvalidId32;

#define ID_16_TYPE(x) struct x { \
    Ids::Id16 id; \
    constexpr x() : id(Ids::InvalidId16) {}; \
    constexpr x(const Ids::Id16 id) : id(id) {}; \
    constexpr explicit operator Ids::Id16() const { return id; } \
    static constexpr x Invalid() { return Ids::InvalidId16; } \
    constexpr uint32_t HashCode() const { return (uint32_t)(id); } \
    const bool operator==(const x& rhs) const { return id == rhs.id; } \
    const bool operator!=(const x& rhs) const { return id != rhs.id; } \
    const bool operator<(const x& rhs) const { return HashCode() < rhs.HashCode(); } \
    const bool operator>(const x& rhs) const { return HashCode() > rhs.HashCode(); } \
    template <typename T> constexpr T As() const { static_assert(sizeof(T) == sizeof(x), "Can only convert between ID types of equal size"); T ret; memcpy((void*)&ret, this, sizeof(T)); return ret; }; \
    }; \
    static constexpr x Invalid##x = Ids::InvalidId16;

#define ID_32_24_8_NAMED_TYPE(x, id32_name, id24_name, id8_name, combined_name) struct x { \
    Ids::Id32 id32_name : 32;\
    union\
    {\
        struct\
        {\
            Ids::Id32 id24_name : 24;\
            Ids::Id32 id8_name: 8;\
        };\
        Ids::Id32 combined_name;\
    };\
    constexpr x() : id32_name(Ids::InvalidId32), id24_name(Ids::InvalidId24), id8_name(Ids::InvalidId8) {};\
    constexpr x(const Ids::Id32 id32, const Ids::Id24 id24, const Ids::Id8 id8) : id32_name(id32), id24_name(id24), id8_name(id8){} \
    constexpr x(const Ids::Id64 id) : id32_name(Ids::Id::GetHigh(id)), id24_name(Ids::Index(Ids::Id::GetLow(id))), id8_name(Ids::Generation(Ids::Id::GetLow(id))) {};\
    explicit constexpr operator Ids::Id64() const { return Ids::Id::MakeId32_24_8(id32_name, id24_name, id8_name); }\
    static constexpr x Invalid() { return Ids::Id::MakeId32_24_8(Ids::InvalidId32, Ids::InvalidId24, Ids::InvalidId8); }\
    constexpr uint32_t HashCode() const { return (uint32_t)combined_name; }\
    constexpr Ids::Id64 HashCode64() const { return Ids::Id::MakeId32_24_8(id32_name, id24_name, id8_name); }\
    const bool operator==(const x& rhs) const { return id32_name == rhs.id32_name && id24_name == rhs.id24_name && id8_name == rhs.id8_name; }\
    const bool operator!=(const x& rhs) const { return id32_name != rhs.id32_name || id24_name != rhs.id24_name || id8_name != rhs.id8_name; }\
    const bool operator<(const x& rhs) const { return HashCode64() < rhs.HashCode64(); }\
    const bool operator>(const x& rhs) const { return HashCode64() > rhs.HashCode64(); }\
    template <typename T> constexpr T As() const { static_assert(sizeof(T) == sizeof(x), "Can only convert between ID types of equal size"); T ret; memcpy((void*)&ret, this, sizeof(T)); return ret; }; \
    }; \
    static constexpr x Invalid##x = Ids::Id::MakeId32_24_8(Ids::InvalidId32, Ids::InvalidId24, Ids::InvalidId8);
#define ID_32_24_8_TYPE(x) ID_32_24_8_NAMED_TYPE(x, parent, index, generation, id)

#define ID_24_8_24_8_NAMED_TYPE(x, id24_0_name, id8_0_name, id24_1_name, id8_1_name, combined0_name, combined1_name) struct x { \
    union\
    {\
        struct\
        {\
            Ids::Id32 id24_0_name : 24;\
            Ids::Id32 id8_0_name : 8;\
        };\
        Ids::Id32 combined0_name;\
    };\
    union\
    {\
        struct\
        {\
            Ids::Id32 id24_1_name : 24;\
            Ids::Id32 id8_1_name : 8;\
        };\
        Ids::Id32 combined1_name;\
    };\
    constexpr x() : combined0_name(Ids::InvalidId32), combined1_name(Ids::InvalidId32) {};\
    constexpr x(const Ids::Id24 id24_0, const Ids::Id8 id8_0, const Ids::Id24 id24_1, const Ids::Id8 id8_1) : id24_0_name(id24_0), id8_0_name(id8_0), id24_1_name(id24_1), id8_1_name(id8_1) {} \
    constexpr x(const Ids::Id64 id) : combined0_name(Ids::Id::GetLow(id)), combined1_name(Ids::Id::GetHigh(id)) {};\
    explicit constexpr operator Ids::Id64() const { return Ids::Id::MakeId24_8_24_8(id24_0_name, id8_0_name, id24_1_name, id8_1_name); }\
    static constexpr x Invalid() { return Ids::InvalidId64; }\
    constexpr Ids::Id32 AllocId() const { return combined1_name; }\
    constexpr uint32_t HashCode() const { return (uint32_t)combined0_name; }\
    constexpr Ids::Id64 HashCode64() const { return Ids::Id::MakeId24_8_24_8(id24_0_name, id8_0_name, id24_1_name, id8_1_name); }\
    const bool operator==(const x& rhs) const { return id24_0_name == rhs.id24_0_name && id8_0_name == rhs.id8_0_name && id24_1_name == rhs.id24_1_name && id8_1_name == rhs.id8_1_name; }\
    const bool operator!=(const x& rhs) const { return id24_0_name != rhs.id24_0_name || id8_0_name != rhs.id8_0_name || id24_1_name != rhs.id24_1_name || id8_1_name != rhs.id8_1_name; }\
    const bool operator<(const x& rhs) const { return HashCode64() < rhs.HashCode64(); }\
    const bool operator>(const x& rhs) const { return HashCode64() > rhs.HashCode64(); }\
    template <typename T> constexpr T As() const { static_assert(sizeof(T) == sizeof(x), "Can only convert between ID types of equal size"); T ret; memcpy((void*)&ret, this, sizeof(T)); return ret; }; \
    }; \
    static constexpr x Invalid##x = Ids::Id::MakeId24_8_24_8(Ids::InvalidId24, Ids::InvalidId8, Ids::InvalidId24, Ids::InvalidId8);
#define ID_24_8_24_8_TYPE(x) ID_24_8_24_8_NAMED_TYPE(x, index0, generation0, index1, generation1, id0, id1)

#define ID_24_8_NAMED_TYPE(x, id24_name, id8_name, combined_name) struct x { \
    union \
    {\
        struct\
        {\
            Ids::Id32 id24_name : 24; \
            Ids::Id32 id8_name : 8; \
        };\
        Ids::Id32 combined_name;\
    }; \
    constexpr x() : id24_name(Ids::InvalidId24), id8_name(Ids::InvalidId8) {} \
    constexpr x(const Ids::Id24 id0, const Ids::Id8 id1) : id24_name(id0), id8_name(id1) {} \
    constexpr x(const Ids::Id32 id) : combined_name(id) {};\
    explicit constexpr operator Ids::Id32() const { return combined_name; }\
    static constexpr x Invalid() { return Ids::InvalidId32; }\
    constexpr uint32_t HashCode() const { return (uint32_t)combined_name; }\
    const bool operator==(const x& rhs) const { return id24_name == rhs.id24_name && id8_name == rhs.id8_name; }\
    const bool operator!=(const x& rhs) const { return id24_name != rhs.id24_name || id8_name != rhs.id8_name; }\
    const bool operator<(const x& rhs) const { return HashCode() < rhs.HashCode(); }\
    const bool operator>(const x& rhs) const { return HashCode() > rhs.HashCode(); }\
    template <typename T> constexpr T As() const { static_assert(sizeof(T) == sizeof(x), "Can only convert between ID types of equal size"); T ret; memcpy((void*)&ret, this, sizeof(T)); return ret; }; \
    }; \
    static constexpr x Invalid##x = Ids::Id::MakeId24_8(Ids::InvalidId24, Ids::InvalidId8);
#define ID_24_8_TYPE(x) ID_24_8_NAMED_TYPE(x, index, generation, id)

namespace Ids
{

typedef uint64_t Id64;
typedef uint32_t Id32;
typedef uint32_t Id24;
typedef uint16_t Id16;
typedef uint8_t Id8;
static constexpr Id64 InvalidId64 = 0xFFFFFFFFFFFFFFFF;
static constexpr Id32 InvalidId32 = 0xFFFFFFFF;
static constexpr Id24 InvalidId24 = 0x00FFFFFF;
static constexpr Id16 InvalidId16 = 0xFFFF;
static constexpr Id8 InvalidId8 = 0xFF;

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
    /// set 16-16 bits id by low and high
    static constexpr Id32 MakeId32(const Id16 high, const Id16 low);
    /// set 24-8 bits in integer
    static constexpr Id32 MakeId24_8(const Id24 big, const Id8 tiny);
    /// set 32-24-8 bits 64 bit integer
    static constexpr Id64 MakeId32_24_8(const Id32 upper, const Id24 big, const Id8 tiny);
    /// set 32-16-16 bits 64 bit integer
    static constexpr Id64 MakeId32_16_16(const Id32 upper, const Id16 high, const Id16 low);
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
Id::MakeId32_16_16(const Id32 upper, const Id16 high, const Id16 low)
{
    return (((uint64_t)upper << 32) & 0xFFFFFFFF00000000) + (((uint64_t)high) & 0x00000000FFFF0000) + (((uint64_t)low << 8) & 0x000000000000FFFF);
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Id64
Id::MakeId24_8_24_8(const Id24 big0, const Id8 tiny0, const Id24 big1, const Id8 tiny1)
{
    return (((uint64_t)big0 << 40) & 0xFFFFFF0000000000) + (((uint64_t)tiny0 << 32) & 0x000000FF00000000) + (((uint64_t)big1 << 8) & 0x00000000FFFFFF00) + (((uint64_t)tiny1) & 0x00000000000000FF);
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
