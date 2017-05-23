//------------------------------------------------------------------------------
//  stringatom.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "util/stringatom.h"
#include "util/localstringatomtable.h"
#include "util/globalstringatomtable.h"

#include <string.h>

namespace Util
{

//------------------------------------------------------------------------------
/**
*/
void
StringAtom::Setup(const char* str)
{
    #if NEBULA3_ENABLE_THREADLOCAL_STRINGATOM_TABLES
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

    #if NEBULA3_ENABLE_THREADLOCAL_STRINGATOM_TABLES
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
    if (0 == this->content)
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
    Compare with raw string. Careful, slow!
*/
bool
StringAtom::operator!=(const char* rhs) const
{
    if (0 == this->content)
    {
        return false;
    }
    else
    {
        return (0 == strcmp(this->content, rhs));
    }
}

} // namespace Util