//------------------------------------------------------------------------------
//  posixguid.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/posix/posixguid.h"
#include <uuid/uuid.h>

namespace Posix
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
PosixGuid::operator=(const String& rhs)
{
    n_assert(rhs.IsValid());
    uuid_t uid;
    int result = uuid_parse(rhs.AsCharPtr(), uid);
    n_assert(-1 != result);
    Memory::Copy(uid, &hi, sizeof(uuid_t));
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::operator==(const PosixGuid& rhs) const
{
    return hi == rhs.hi && lo == rhs.lo;
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::operator!=(const PosixGuid& rhs) const
{
    return !(rhs == *this);
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::operator<(const PosixGuid& rhs) const
{
    if (hi < rhs.hi)
    {
        return true;
    }
    else if (hi == rhs.hi)
    {
        return lo < rhs.lo;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::operator<=(const PosixGuid& rhs) const
{
    return *this < rhs || *this == rhs;
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::operator>(const PosixGuid& rhs) const
{
    if (hi > rhs.hi)
    {
        return true;
    }
    else if (hi == rhs.hi)
    {
        return lo > rhs.lo;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::operator>=(const PosixGuid& rhs) const
{
    return *this > rhs || *this == rhs;
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::IsValid() const
{
    return lo > 0 || hi > 0;
}

//------------------------------------------------------------------------------
/**
*/
void
PosixGuid::Generate()
{
    uuid_t uid;
    uuid_generate(uid);
    Memory::Copy(uid, &hi, sizeof(uuid_t));
}

//------------------------------------------------------------------------------
/**
*/
String
PosixGuid::AsString() const
{
    char uuidStr[37];
    uuid_unparse((const unsigned char*)&this->hi, uuidStr);
    String result = uuidStr;
    return result;
}

//------------------------------------------------------------------------------
/**
    This method allows read access to the raw binary data of the uuid.
    It returns the number of bytes in the buffer, and a pointer to the
    data.
*/
SizeT
PosixGuid::AsBinary(const unsigned char*& outPtr) const
{
    outPtr = (const unsigned char*) &this->hi;
    return sizeof(uuid_t);
}

//------------------------------------------------------------------------------
/**
*/
PosixGuid
PosixGuid::FromString(const Util::String& str)
{
    PosixGuid newGuid;
    uuid_t uid;
    int success = uuid_parse(str.AsCharPtr(), uid);
    n_assert(0 == success);
    Memory::Copy(uid, &newGuid.hi, sizeof(uuid_t));
    return newGuid;
}

//------------------------------------------------------------------------------
/**
    Constructs the guid from binary data, as returned by the AsBinary().
*/
PosixGuid
PosixGuid::FromBinary(const unsigned char* ptr, SizeT numBytes)
{
    n_assert((0 != ptr) && (numBytes == sizeof(uint64_t) * 2));
    PosixGuid newGuid;
    Memory::Copy(ptr, &(newGuid.hi), sizeof(uuid_t));
    return newGuid;
}

//------------------------------------------------------------------------------
/**
    This method returns a hash code for the uuid, compatible with 
    Util::HashTable.
*/
IndexT
PosixGuid::HashCode() const
{
    return (IndexT)(hi ^ lo);
}

}; // namespace Posix
