//------------------------------------------------------------------------------
//  posixguid.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/posix/posixguid.h"

namespace Posix
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
PosixGuid::operator=(const PosixGuid& rhs)
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
PosixGuid::operator=(const String& rhs)
{
    n_assert(rhs.IsValid());
    int result = uuid_parse(rhs.AsCharPtr(), this->uuid);
    n_assert(0 != result);
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::operator==(const PosixGuid& rhs) const
{
    int result = uuid_compare(this->uuid, rhs.uuid);
    return (0 == result);
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::operator!=(const PosixGuid& rhs) const
{
    int result = uuid_compare(this->uuid, rhs.uuid);
    return (0 != result);
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::operator<(const PosixGuid& rhs) const
{
    int result = uuid_compare(this->uuid, rhs.uuid);
    return (-1 == result);
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::operator<=(const PosixGuid& rhs) const
{
    int result = uuid_compare(this->uuid, rhs.uuid);
    return ((-1 == result) || (0 == result));
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::operator>(const PosixGuid& rhs) const
{
    int result = uuid_compare(this->uuid, rhs.uuid);
    return (1 == result);
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::operator>=(const PosixGuid& rhs) const
{
    int result = uuid_compare(this->uuid, rhs.uuid);
    return ((1 == result) || (0 == result));
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixGuid::IsValid() const
{
    int result = uuid_is_null(this->uuid);
    return (0 == result);
}

//------------------------------------------------------------------------------
/**
*/
void
PosixGuid::Generate()
{
    uuid_generate(this->uuid);
}

//------------------------------------------------------------------------------
/**
*/
String
PosixGuid::AsString() const
{
    char uuidStr[37];
    uuid_unparse(this->uuid, uuidStr);
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
    outPtr = (const unsigned char*) &this->uuid;
    return sizeof(uuid_t);
}

//------------------------------------------------------------------------------
/**
*/
PosixGuid
PosixGuid::FromString(const Util::String& str)
{
    PosixGuid newGuid;
    int success = uuid_parse(str.AsCharPtr(), newGuid.uuid);
    n_assert(0 == success);
    return newGuid;
}

//------------------------------------------------------------------------------
/**
    Constructs the guid from binary data, as returned by the AsBinary().
*/
PosixGuid
PosixGuid::FromBinary(const unsigned char* ptr, SizeT numBytes)
{
    n_assert((0 != ptr) && (numBytes == sizeof(uuid_t)));
    PosixGuid newGuid;
    Memory::Copy(ptr, &(newGuid.uuid), sizeof(uuid_t));
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
    const uint64_t* half = reinterpret_cast<const uint64_t*>(this->uuid);
    return (IndexT)(half[0] ^ half[1]);
}

}; // namespace Posix
