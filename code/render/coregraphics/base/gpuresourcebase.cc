//------------------------------------------------------------------------------
//  gpuresourcebase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/gpuresourcebase.h"

namespace Base
{
__ImplementClass(Base::GpuResourceBase, 'RSBS', Resources::Resource);

//------------------------------------------------------------------------------
/**
*/
GpuResourceBase::GpuResourceBase() :
    usage(UsageImmutable),
    access(AccessNone),
	syncing(SyncingFlush)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
GpuResourceBase::~GpuResourceBase()
{
    // empty
}

} // namespace Base