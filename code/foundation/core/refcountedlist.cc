//------------------------------------------------------------------------------
//  refcountedlist.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcountedlist.h"
#include "core/refcounted.h"
#include "core/sysfunc.h"

namespace Core
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
RefCountedList::DumpLeaks()
{
    if (!this->IsEmpty())
    {
        SysFunc::DebugOut("\n\n\n******** NEBULA T REFCOUNTING LEAKS DETECTED:\n\n");
        RefCountedList::Iterator iter;
        for (iter = this->Begin(); iter != this->End(); iter++)
        {
			if (this->refcountedDebugNames.Contains(*iter))
			{
				String msg;
				msg.Format("*** NEBULA T REFCOUNT LEAK: Object of class '%s' with debug identifier '%s' at address '0x%08lx', refcount is '%d'\n",
					(*iter)->GetClassName().AsCharPtr(),
					this->refcountedDebugNames[*iter].AsCharPtr(),
					(*iter),
					(*iter)->GetRefCount());
				SysFunc::DebugOut(msg.AsCharPtr());
			}
			else
			{
				String msg;
				msg.Format("*** NEBULA T REFCOUNT LEAK: Object of class '%s' at address '0x%08lx', refcount is '%d'\n",
					(*iter)->GetClassName().AsCharPtr(),
					(*iter),
					(*iter)->GetRefCount());
				SysFunc::DebugOut(msg.AsCharPtr());
			}            
        }
        SysFunc::DebugOut("\n******** END OF NEBULA T REFCOUNT LEAK REPORT\n\n\n");
		this->refcountedDebugNames.Clear();
    }
    else
    {
        SysFunc::DebugOut("\n>>> HOORAY, NO NEBULA T REFCOUNT LEAKS!!!\n\n\n");
    }
}

//------------------------------------------------------------------------------
/**
*/
void
RefCountedList::SetDebugName(RefCounted* ptr, const Util::String& name)
{
	n_assert(!this->refcountedDebugNames.Contains(ptr));
	this->refcountedDebugNames.Add(ptr, name);
}

} // namespace Core
