#pragma once
//------------------------------------------------------------------------------
/**
    @class OSX::OSXGuid
 
    OSX implementation of the Util::Guid class.
 
    (C) 2010 Radon Labs GmbH
 */
#include "core/types.h"
#include "util/string.h"
#include "core/sysfunc.h"

//------------------------------------------------------------------------------
namespace OSX
{
class OSXGuid
{
public:
    /// override new operator
    void* operator new(size_t s);
    /// override delete operator
    void operator delete(void* ptr);
    
    /// constructor
    OSXGuid();
    /// copy constructor
    OSXGuid(const OSXGuid& rhs);
    /// construct from raw binary data as returned by AsBinary()
    OSXGuid(const unsigned char* ptr, SizeT size);
    /// assignement operator
    void operator=(const OSXGuid& rhs);
    /// assignment operator from string
    void operator=(const Util::String& rhs);
    /// equality operator
    bool operator==(const OSXGuid& rhs) const;
    /// inequlality operator
    bool operator!=(const OSXGuid& rhs) const;
    /// less-then operator
    bool operator<(const OSXGuid& rhs) const;
    /// less-or-equal operator
    bool operator<=(const OSXGuid& rhs) const;
    /// greater-then operator
    bool operator>(const OSXGuid& rhs) const;
    /// greater-or-equal operator
    bool operator>=(const OSXGuid& rhs) const;
    /// return true if the contained guid is valid (not NIL)
    bool IsValid() const;
    /// generate a new guid
    void Generate();
    /// construct from string representation
    static OSXGuid FromString(const Util::String& str);
    /// construct from binary representation
    static OSXGuid FromBinary(const unsigned char* ptr, SizeT numBytes);
    /// get as string
    Util::String AsString() const;
    /// get pointer to binary data
    SizeT AsBinary(const unsigned char*& outPtr) const;
    /// return the size of the binary representation in bytes
    static SizeT BinarySize();
    /// get a hash code (compatible with Util::HashTable)
    IndexT HashCode() const;
    
private:
    uuid_t uuid;
};
    
//------------------------------------------------------------------------------
/**
 */
__forceinline void*
OSXGuid::operator new(size_t size)
{
    #if NEBULA3_DEBUG
    n_assert(size == sizeof(OSXGuid));
    #endif
    
    #if NEBULA3_OBJECTS_USE_MEMORYPOOL
    return Memory::ObjectPoolAllocator->Alloc(size);
    #else
    return Memory::Alloc(Memory::ObjectHeap, size);
    #endif
}
    
//------------------------------------------------------------------------------
/**
 */
__forceinline void
OSXGuid::operator delete(void* ptr)
{
    #if NEBULA3_OBJECTS_USE_MEMORYPOOL
    return Memory::ObjectPoolAllocator->Free(ptr, sizeof(OSXGuid));
    #else
    return Memory::Free(Memory::ObjectHeap, ptr);
    #endif
}
    
//------------------------------------------------------------------------------
/**
 */
inline
OSXGuid::OSXGuid()
{
    uuid_clear(this->uuid);
}
    
//------------------------------------------------------------------------------
/**
 */
inline
OSXGuid::OSXGuid(const OSXGuid& rhs)
{
    uuid_copy(this->uuid, rhs.uuid);
}
    
//------------------------------------------------------------------------------
/**
 */
inline
OSXGuid::OSXGuid(const unsigned char* ptr, SizeT size)
{
    n_assert((0 != ptr) && (size == sizeof(uuid_t)));
    uuid_copy(this->uuid, *(uuid_t*)ptr);
}
    
//------------------------------------------------------------------------------
/**
 */
inline SizeT
OSXGuid::BinarySize()
{
    return sizeof(uuid_t);
}
    
} // namespace Win32
//------------------------------------------------------------------------------
