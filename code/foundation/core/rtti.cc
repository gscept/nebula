//------------------------------------------------------------------------------
//  rtti.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/rtti.h"
#include "core/refcounted.h"
#include "core/sysfunc.h"
#include "memory/poolarrayallocator.h"

namespace Core
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
Rtti::Construct(const char* className, FourCC fcc, Creator creatorFunc, const Rtti* parentClass, SizeT instSize)
{
    // make sure String, etc... is working correctly
    Core::SysFunc::Setup();

    // NOTE: FourCC code may be 0!
    n_assert(0 != className);
    n_assert(parentClass != this);

    // setup members
    this->parent = parentClass;
    this->fourCC = fcc;     // NOTE: may be 0
    this->creator = creatorFunc;
    this->instanceSize = instSize;

    // register class with factory
    this->name = className;
    if (fcc.IsValid())
    {
        // FourCC code valid
        if (!Factory::Instance()->ClassExists(fcc))
        {
            Factory::Instance()->Register(this, this->name, fcc);        
        }
        // make a debug check that no name/fcc collission occured, but only in debug mode
        #if NEBULA3_DEBUG
        else
        {
            const Rtti* checkRtti = Factory::Instance()->GetClassRtti(fcc);
            n_assert(0 != checkRtti);
            if (checkRtti != this)
            {
                n_error("Class registry collision: (%s, %s) collides with (%s, %s)!",
                    this->name.AsCharPtr(), this->fourCC.AsString().AsCharPtr(),
                    checkRtti->name.AsCharPtr(), checkRtti->fourCC.AsString().AsCharPtr());
            }
        }
        #endif
    }
    else
    {
        // FourCC code not valid
        if (!Factory::Instance()->ClassExists(this->name))
        {
            Factory::Instance()->Register(this, this->name);
        }
        // make a debug check that no name/fcc collission occured, but only in debug mode
        #if NEBULA3_DEBUG
        else
        {
            const Rtti* checkRtti = Factory::Instance()->GetClassRtti(this->name);
            n_assert(0 != checkRtti);
            if (checkRtti != this)
            {
                n_error("Class registry collision: (%s) collides with (%s)!",
                    this->name.AsCharPtr(), checkRtti->name.AsCharPtr());
            }
        }
        #endif
    }
}

//------------------------------------------------------------------------------
/**
*/
Rtti::Rtti(const char* className, FourCC fcc, Creator creatorFunc, const Rtti* parentClass, SizeT instSize)
{
    this->Construct(className, fcc, creatorFunc, parentClass, instSize);
}

//------------------------------------------------------------------------------
/**
*/
Rtti::Rtti(const char* className, Creator creatorFunc, const Rtti* parentClass, SizeT instSize)
{
    this->Construct(className, 0, creatorFunc, parentClass, instSize);
}

//------------------------------------------------------------------------------
/**
*/
RefCounted*
Rtti::Create() const
{
    if (0 == this->creator)
    {
        n_error("Rtti::Create(): Trying to create instance of abstract class '%s'!", this->name.AsCharPtr());
        return 0;
    }
    else
    {
        return this->creator();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
Rtti::IsDerivedFrom(const Rtti& other) const
{
    const Rtti* cur;
    for (cur = this; cur != 0; cur = cur->GetParent())
    {
        if (cur == &other)
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
Rtti::IsDerivedFrom(const Util::String& otherClassName) const
{
    const Rtti* cur;
    for (cur = this; cur != 0; cur = cur->GetParent())
    {
        if (cur->name == otherClassName)
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
Rtti::IsDerivedFrom(const Util::FourCC& otherClassFourCC) const
{
    const Rtti* cur;
    for (cur = this; cur != 0; cur = cur->GetParent())
    {
        if (cur->fourCC == otherClassFourCC)
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void*
Rtti::AllocInstanceMemory()
{
    #if NEBULA3_OBJECTS_USE_MEMORYPOOL    
    void* ptr = Memory::ObjectPoolAllocator->Alloc(this->instanceSize);
    #else
    void* ptr = Memory::Alloc(Memory::ObjectHeap, this->instanceSize);
    #endif
    return ptr;
}

//------------------------------------------------------------------------------
/**
*/
void
Rtti::FreeInstanceMemory(void* ptr)
{
    #if NEBULA3_OBJECTS_USE_MEMORYPOOL
    Memory::ObjectPoolAllocator->Free(ptr, this->instanceSize);
    #else
    Memory::Free(Memory::ObjectHeap, ptr);
    #endif
}

} // namespace Core
