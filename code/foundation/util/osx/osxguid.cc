//------------------------------------------------------------------------------
//  osxguid.cc
//  (C) 2010 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "osxguid.h"

namespace OSX
{
using namespace Util;
    
//------------------------------------------------------------------------------
/**
 */
void
OSXGuid::operator=(const OSXGuid& rhs)
{
    if (this != &rhs)
    {
        uuid_copy(this->uuid, rhs.uuid);
    }
}
    
//------------------------------------------------------------------------------
/**
 */
void
OSXGuid::operator=(const String& rhs)
{
    n_assert(rhs.IsValid());
    n_assert((rhs.Length() + 1) == sizeof(uuid_string_t));
    int res = uuid_parse(rhs.AsCharPtr(), this->uuid);
    n_assert(0 == res);
}
    
//------------------------------------------------------------------------------
/**
 */
bool
OSXGuid::operator==(const OSXGuid& rhs) const
{
    return (uuid_compare(this->uuid, rhs.uuid) == 0);
}
    
//------------------------------------------------------------------------------
/**
 */
bool
OSXGuid::operator!=(const OSXGuid& rhs) const
{
    return (uuid_compare(this->uuid, rhs.uuid) != 0);
}
                
//------------------------------------------------------------------------------
/**
 */
bool
OSXGuid::operator<(const OSXGuid& rhs) const
{
    return (uuid_compare(this->uuid, rhs.uuid) < 0);
}
    
//------------------------------------------------------------------------------
/**
 */
bool
OSXGuid::operator<=(const OSXGuid& rhs) const
{
    return (uuid_compare(this->uuid, rhs.uuid) <= 0);
}
    
//------------------------------------------------------------------------------
/**
 */
bool
OSXGuid::operator>(const OSXGuid& rhs) const
{
    return (uuid_compare(this->uuid, rhs.uuid) > 0);
}
    
//------------------------------------------------------------------------------
/**
 */
bool
OSXGuid::operator>=(const OSXGuid& rhs) const
{
    return (uuid_compare(this->uuid, rhs.uuid) >= 0);
}
    
//------------------------------------------------------------------------------
/**
 */
bool
OSXGuid::IsValid() const
{
    return uuid_is_null(this->uuid);
}
    
//------------------------------------------------------------------------------
/**
 */
void
OSXGuid::Generate()
{
    uuid_generate(this->uuid);
}
    
//------------------------------------------------------------------------------
/**
 */
String
OSXGuid::AsString() const
{
    uuid_string_t uuidStr;
    uuid_unparse(this->uuid, uuidStr);
    String result(uuidStr);
    return result;
}
    
//------------------------------------------------------------------------------
/**
    This method allows read access to the raw binary data of the uuid.
    It returns the number of bytes in the buffer, and a pointer to the
    data.
*/
SizeT
OSXGuid::AsBinary(const unsigned char*& outPtr) const
{
    outPtr = (const unsigned char*) &this->uuid;
    return sizeof(uuid_t);
}
    
//------------------------------------------------------------------------------
/**
*/
OSXGuid
OSXGuid::FromString(const Util::String& str)
{
    OSXGuid newGuid;
    newGuid = str;
    return newGuid;
}
    
//------------------------------------------------------------------------------
/**
    Constructs the guid from binary data, as returned by the AsBinary().
*/
OSXGuid
OSXGuid::FromBinary(const unsigned char* ptr, SizeT numBytes)
{
    n_assert((0 != ptr) && (numBytes == sizeof(uuid_t)));
    OSXGuid newGuid(ptr, numBytes);
    return newGuid;
}
    
//------------------------------------------------------------------------------
/**
    This method returns a hash code for the uuid, compatible with 
    Util::HashTable.
    This is simply copied from String::HashCode...
*/
IndexT
OSXGuid::HashCode() const
{
    IndexT hash = 0;
    const unsigned char* ptr = (const unsigned char*) &(this->uuid);
    SizeT len = sizeof(this->uuid);
    IndexT i;
    for (i = 0; i < len; i++)
    {
        hash += ptr[i];
        hash += hash << 10;
        hash ^= hash >>  6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    hash &= ~(1<<31);       // don't return a negative number (in case IndexT is defined signed)
    return hash;
}
    
} // namespace OSX
