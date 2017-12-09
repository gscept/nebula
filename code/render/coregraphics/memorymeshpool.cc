//------------------------------------------------------------------------------
//  memorymeshloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/memorymeshpool.h"
#include "coregraphics/mesh.h"
#include "coregraphics/legacy/nvx2streamreader.h"

namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryMeshPool, 'DMML', Resources::ResourceMemoryPool);

using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
MemoryMeshPool::MemoryMeshPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
MemoryMeshPool::~MemoryMeshPool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ResourcePool::LoadStatus
MemoryMeshPool::LoadFromMemory(const Ids::Id24 id, void* info)
{
	MeshCreateInfo* data = (MeshCreateInfo*)info;
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryMeshPool::Unload(const Ids::Id24 id)
{
}

} // namespace CoreGraphics
