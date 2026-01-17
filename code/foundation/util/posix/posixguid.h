#pragma once
#ifndef POSIX_POSIXGUID_H
#define POSIX_POSIXGUID_H
//------------------------------------------------------------------------------
/**
    @class Posix::PosixGuid
    
    Posix implementation of the Util::Guid class. GUIDs can be
    compared and provide a hash code, so they can be used as keys in
    most collections.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixGuid
{
public:    
    /// constructor
    PosixGuid();
    /// construct from raw binary data as returned by AsBinary()
    PosixGuid(const unsigned char* ptr, SizeT size);
    /// assignment operator from string
    void operator=(const Util::String& rhs);
    /// equality operator
    bool operator==(const PosixGuid& rhs) const;
    /// inequlality operator
    bool operator!=(const PosixGuid& rhs) const;
    /// less-then operator
    bool operator<(const PosixGuid& rhs) const;
    /// less-or-equal operator
    bool operator<=(const PosixGuid& rhs) const;
    /// greater-then operator
    bool operator>(const PosixGuid& rhs) const;
    /// greater-or-equal operator
    bool operator>=(const PosixGuid& rhs) const;
    /// return true if the contained guid is valid (not NIL)
    bool IsValid() const;
    /// generate a new guid
    void Generate();
    /// construct from string representation
    static PosixGuid FromString(const Util::String& str);
    /// construct from binary representation
    static PosixGuid FromBinary(const unsigned char* ptr, SizeT numBytes);
    /// get as string
    Util::String AsString() const;
    /// get pointer to binary data
    SizeT AsBinary(const unsigned char*& outPtr) const;
    /// get a hash code (compatible with Util::HashTable)
    uint32_t HashCode() const;

//private:    
    uint64_t hi;
    uint64_t lo;
};

//------------------------------------------------------------------------------
/**
*/
inline
PosixGuid::PosixGuid()
{
    hi = 0UL;
    lo = 0UL;
}

//------------------------------------------------------------------------------
/**
*/
inline
PosixGuid::PosixGuid(const unsigned char* ptr, SizeT size)
{
    n_assert((0 != ptr) && (size == sizeof(uint64_t) * 2));
    Memory::Copy(ptr, &this->hi, sizeof(uint64_t) * 2);
}

}; // namespace Posix
//------------------------------------------------------------------------------
#endif

