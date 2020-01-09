//------------------------------------------------------------------------------
//  stringatom.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/stringatom.h"
#include "util/localstringatomtable.h"
#include "util/globalstringatomtable.h"

#include <string.h>

//------------------------------------------------------------------------------
/**
	Literal constructor form string, to use "foobar"_atm will automatically construct a StringAtom
*/
Util::StringAtom
operator ""_atm(const char* c, std::size_t s)
{
	return Util::StringAtom(c);
}

namespace Util
{

//------------------------------------------------------------------------------
/**
*/
void
StringAtom::Setup(const char* str)
{
    #if NEBULA_ENABLE_THREADLOCAL_STRINGATOM_TABLES
        // first check our thread-local string atom table whether the string
        // is already registered there, this does not require any thread
        // synchronisation
        LocalStringAtomTable* localTable = LocalStringAtomTable::Instance();
        this->content = localTable->Find(str);
        if (0 != this->content)
        {
            // yep, the string is in the local table, we're done
            return;
        }
    #endif

    // the string wasn't in the local table (or thread-local tables are disabled), 
    // so we need to check the global table, this involves thread synchronisation
    GlobalStringAtomTable* globalTable = GlobalStringAtomTable::Instance();
    globalTable->Lock();
    this->content = globalTable->Find(str);
    if (0 == this->content)
    {
        // hrmpf, string isn't in global table either yet, so add it...
        this->content = globalTable->Add(str);
    }
    globalTable->Unlock();

    #if NEBULA_ENABLE_THREADLOCAL_STRINGATOM_TABLES
        // finally, add the new string to our local table as well, so the
        // next lookup from our thread of this string will be faster
        localTable->Add(this->content);
    #endif
}

//------------------------------------------------------------------------------
/**
    Compare with raw string. Careful, slow!
*/
bool
StringAtom::operator==(const char* rhs) const
{
    if (nullptr == this->content)
    {
        return false;
    }
    else
    {
        return (0 == strcmp(this->content, rhs));
    }
}

//------------------------------------------------------------------------------
/**
*/
bool 
StringAtom::operator==(nullptr_t) const
{
	return this->content == nullptr;
}

//------------------------------------------------------------------------------
/**
    Compare with raw string. Careful, slow!
*/
bool
StringAtom::operator!=(const char* rhs) const
{
    if (nullptr == this->content)
    {
        return false;
    }
    else
    {
        return (0 == strcmp(this->content, rhs));
    }
}

//------------------------------------------------------------------------------
/**
*/
IndexT
StringAtom::StringHashCode() const
{
    IndexT hash = 0;
    const char* ptr = this->content;
    size_t len = strlen(ptr);
    IndexT i;
    for (i = 0; i < len; i++)
    {
        hash += ptr[i];
        hash += hash << 10;
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    hash &= ~(1 << 31);       // don't return a negative number (in case IndexT is defined signed)
    return hash;
}

//------------------------------------------------------------------------------
/**
*/
Util::StringAtom
operator""_atm(const char* c)
{
	return Util::StringAtom();
}

} // namespace Util