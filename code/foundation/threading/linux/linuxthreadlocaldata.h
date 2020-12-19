#pragma once
//------------------------------------------------------------------------------
/**
 *  @class Linux::LinuxThreadLocalData
 *
 *  Thread local data storage class for platforms which don't have a proper
 *  __thread implementation but a pthread implementation.
 *
 *  (C) 2011 A.Weissflog
 *  (C) 2013-2018 Individual contributors, see AUTHORS file
 */
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Linux
{
class LinuxThreadLocalData
{
public:
    /// setup the object from the main thread (call from SysFunc::Setup())
    static void SetupMainThread();

    /// register a new slot, this is usually called from the main thread
    static IndexT RegisterSlot();
    /// set thread local pointer value (must only be called once)
    static void SetPointer(IndexT slot, void* ptr);
    /// get thread local pointer value
    static void* GetPointer(IndexT slot);
    /// clear pointer (must only be called after SetPointer)
    static void ClearPointer(IndexT slot);

private:
    /// get pointer to thread locale data table, create table if this is the first call for this thread
    static void** GetThreadLocalTable();

    static const int MaxNumSlots = 4096;
    static pthread_key_t key;
    static IndexT volatile slot;
};

} // namespace Linux
//------------------------------------------------------------------------------
