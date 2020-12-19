//------------------------------------------------------------------------------
//  linuxthreadlocaldata.cc
//  (C) 2011 A.Weissflog
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "linuxthreadlocaldata.h"
#include "threading/linux/linuxinterlocked.h"

namespace Linux
{
pthread_key_t LinuxThreadLocalData::key = 0;
IndexT volatile LinuxThreadLocalData::slot = 0;

//------------------------------------------------------------------------------
/**
    Allocates a new thread-specific data block and sets it as thread-local
    data. This is called from SetPointer(), GetPointer() and the other
    accessor functions.
*/
void**
LinuxThreadLocalData::GetThreadLocalTable()
{
    n_assert(0 != key);
    void** table = (void**) pthread_getspecific(key);
    if (table == 0)
    {
        // first call for this thread, allocate thread-local table
        n_dbgout("LinuxThreadLocalData::GetThreadLocalTable(): first call for this thread, allocating new table\n");
        int tableSize = MaxNumSlots * sizeof(void*);
        table = (void**) Memory::Alloc(Memory::DefaultHeap, tableSize);
        n_assert(0 != table);
        Memory::Clear(table, tableSize);
        pthread_setspecific(key, table);
        n_assert(pthread_getspecific(key) == table);
    }
    n_assert(0 != table);
    return table;
}

//------------------------------------------------------------------------------
/**
 *  This method must be called once from the main thread (usually
 *  from SysFunc::Setup().
 */
void
LinuxThreadLocalData::SetupMainThread()
{
    n_assert(0 == key);
    n_assert(0 == slot);
    n_printf("LinuxThreadLocalData::SetupMainThread() called!\n");

    // create the thread-local key
    int res = pthread_key_create(&key, 0);
    n_assert(0 == res);
}

//------------------------------------------------------------------------------
/**
    Register a new slot in the thread-locale table. This is usually called
    once from the main-thread per thread-local object.
*/
IndexT
LinuxThreadLocalData::RegisterSlot()
{
    n_assert(slot < (MaxNumSlots - 1));
    return LinuxInterlocked::Increment(slot);
}

//------------------------------------------------------------------------------
/**
    Set a thread-local pointer for a slot. This method must only be called
    once per thread.
*/
void
LinuxThreadLocalData::SetPointer(IndexT slot, void* ptr)
{
    void** table = LinuxThreadLocalData::GetThreadLocalTable();
    n_assert((slot >= 0) && (slot < MaxNumSlots));
    n_assert(0 == table[slot]);
    table[slot] = ptr;
}

//------------------------------------------------------------------------------
/**
    Get a thread-local pointer for a slot.
*/
void*
LinuxThreadLocalData::GetPointer(IndexT slot)
{
    void** table = LinuxThreadLocalData::GetThreadLocalTable();
    n_assert((slot >= 0) && (slot < MaxNumSlots));
    return table[slot];
}

//------------------------------------------------------------------------------
/**
    Clear thread-local pointer for a slot.
*/
void
LinuxThreadLocalData::ClearPointer(IndexT slot)
{
    void** table = LinuxThreadLocalData::GetThreadLocalTable();
    n_assert((slot >= 0) && (slot < MaxNumSlots));
    table[slot] = 0;
}

} // namespace Linux
