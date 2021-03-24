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
#include <uuid/uuid.h>

//------------------------------------------------------------------------------
namespace Posix
{
class PosixGuid
{
public:    
    /// overloaded operator new
    void* operator new(size_t size);
    /// overloaded operator delete
    void operator delete(void* p);

    /// constructor
    PosixGuid();
    /// copy constructor
    PosixGuid(const PosixGuid& rhs);
    /// construct from raw binary data as returned by AsBinary()
    PosixGuid(const unsigned char* ptr, SizeT size);
    /// assignement operator
    void operator=(const PosixGuid& rhs);
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
    IndexT HashCode() const;

private:    
    uuid_t uuid;
};

//------------------------------------------------------------------------------
/**
*/
inline
void*
PosixGuid::operator new(size_t size)
{
    return Memory::Alloc(Memory::ObjectHeap, size);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
PosixGuid::operator delete(void* p)
{
    return Memory::Free(Memory::ObjectHeap, p);
}

//------------------------------------------------------------------------------
/**
*/
inline
PosixGuid::PosixGuid()
{
    uuid_clear(this->uuid);
}

//------------------------------------------------------------------------------
/**
*/
inline
PosixGuid::PosixGuid(const PosixGuid& rhs)
{
    uuid_copy(this->uuid, rhs.uuid);
}

//------------------------------------------------------------------------------
/**
*/
inline
PosixGuid::PosixGuid(const unsigned char* ptr, SizeT size)
{
    n_assert((0 != ptr) && (size == sizeof(uuid_t)));
    Memory::Copy(ptr, &this->uuid, sizeof(uuid_t));
}

}; // namespace Posix
//------------------------------------------------------------------------------
#endif

