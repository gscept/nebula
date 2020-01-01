//------------------------------------------------------------------------------
//  globalstringatomtable.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/globalstringatomtable.h"

namespace Util
{
__ImplementInterfaceSingleton(Util::GlobalStringAtomTable);

//------------------------------------------------------------------------------
/**
*/
GlobalStringAtomTable::GlobalStringAtomTable()
{
    __ConstructInterfaceSingleton;
    
    // setup the global string buffer
    this->stringBuffer.Setup(NEBULA_GLOBAL_STRINGBUFFER_CHUNKSIZE);
}

//------------------------------------------------------------------------------
/**
*/
GlobalStringAtomTable::~GlobalStringAtomTable()
{
    this->critSect.Enter();
    this->stringBuffer.Discard();
    __DestructInterfaceSingleton;
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
GlobalStringAtomTable::Lock()
{
    this->critSect.Enter();
}

//------------------------------------------------------------------------------
/**
*/
void
GlobalStringAtomTable::Unlock()
{
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
    This adds a new string to the atom table and the global string buffer,
    and returns the pointer to the string in the string buffer.

    NOTE: you MUST call this method within Lock()/Unlock()!
*/
const char*
GlobalStringAtomTable::Add(const char* str)
{
    StaticString sstr;
    sstr.ptr = (char*)this->stringBuffer.AddString(str);
    this->table.InsertSorted(sstr);
    return sstr.ptr;
}

//------------------------------------------------------------------------------
/**
    Debug method: get an array with all string in the table.
*/
GlobalStringAtomTable::DebugInfo
GlobalStringAtomTable::GetDebugInfo() const
{
    this->critSect.Enter();
    DebugInfo debugInfo;
    debugInfo.strings.Reserve(this->table.Size());
    debugInfo.chunkSize = NEBULA_GLOBAL_STRINGBUFFER_CHUNKSIZE;
    debugInfo.numChunks = this->stringBuffer.GetNumChunks();
    debugInfo.allocSize = debugInfo.chunkSize * debugInfo.numChunks;
    debugInfo.usedSize  = 0;
    debugInfo.growthEnabled = NEBULA_ENABLE_GLOBAL_STRINGBUFFER_GROWTH;

    IndexT i;
    for (i = 0; i < table.Size(); i++)
    {        
        const char* str = this->table[i].ptr;
        debugInfo.strings.Append(this->table[i].ptr);
        debugInfo.usedSize += strlen(str) + 1;
    }
    this->critSect.Leave();
    return debugInfo;
}

} // namespace Util