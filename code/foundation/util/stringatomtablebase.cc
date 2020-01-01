//------------------------------------------------------------------------------
//  stringatomtablebase.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/stringatomtablebase.h"

#include <string.h>

namespace Util
{

//------------------------------------------------------------------------------
/**
*/
StringAtomTableBase::StringAtomTableBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
StringAtomTableBase::~StringAtomTableBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
const char*
StringAtomTableBase::Find(const char* str) const
{
    StaticString sstr;
    sstr.ptr = (char*)str;
    IndexT i = this->table.BinarySearchIndex(sstr);
    if (InvalidIndex == i)
    {
        return 0;
    }
    else
    {
        return this->table[i].ptr;

    }
}    

//------------------------------------------------------------------------------
/**
*/
bool
StringAtomTableBase::StaticString::operator==(const StaticString& rhs) const
{
    return (strcmp(this->ptr, rhs.ptr) == 0);
}

//------------------------------------------------------------------------------
/**
*/
bool
StringAtomTableBase::StaticString::operator!=(const StaticString& rhs) const
{
    return (strcmp(this->ptr, rhs.ptr) != 0);
}

//------------------------------------------------------------------------------
/**
*/
bool
StringAtomTableBase::StaticString::operator<(const StaticString& rhs) const
{
    return (strcmp(this->ptr, rhs.ptr) < 0);
}

//------------------------------------------------------------------------------
/**
*/
bool
StringAtomTableBase::StaticString::operator>(const StaticString& rhs) const
{
    return (strcmp(this->ptr, rhs.ptr) > 0);
}

} // namespace Util