#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::GlobalStringAtomTable
  
    Global string atom table. This is the definitive string atom table which
    contains the string of all string atoms of all threads. Locking
    is necessary to lookup or add a string (that's why thread-local
    string atom tables exist as local cache to prevent too much locking of
    the global table).
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "util/stringatomtablebase.h"
#include "core/singleton.h"
#include "threading/criticalsection.h"
#include "util/stringbuffer.h"

//------------------------------------------------------------------------------
namespace Util
{
class GlobalStringAtomTable : public StringAtomTableBase
{
    __DeclareInterfaceSingleton(GlobalStringAtomTable);
public:
    /// constructor
    GlobalStringAtomTable();
    /// destructor
    ~GlobalStringAtomTable();

    /// get pointer to global string buffer
    StringBuffer* GetGlobalStringBuffer() const;

    /// debug functionality: DebugInfo struct
    struct DebugInfo
    {
        Util::Array<const char*> strings;
        size_t chunkSize;
		size_t numChunks;
		size_t allocSize;
		size_t usedSize;
        bool growthEnabled;
    };
    
    /// debug functionality: get copy of the string atom table
    DebugInfo GetDebugInfo() const;

private:
    friend class StringAtom;

    /// take the critical section
    void Lock();
    /// add a string pointer to the atom table and string buffer (must be called inside Lock())
    const char* Add(const char* str);
    /// release the critical section
    void Unlock();

    Threading::CriticalSection critSect;
    StringBuffer stringBuffer;
};

} // namespace Util
//------------------------------------------------------------------------------

