//------------------------------------------------------------------------------
//  memorymeshloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/memorymeshloader.h"
#include "coregraphics/mesh.h"
#include "coregraphics/legacy/nvx2streamreader.h"

namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryMeshLoader, 'DMML', Resources::ResourceLoader);

using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
MemoryMeshLoader::MemoryMeshLoader() :
    usage(Base::GpuResourceBase::UsageImmutable),
    access(Base::GpuResourceBase::AccessNone)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
MemoryMeshLoader::OnLoadRequested()
{
    n_assert(this->GetState() == Resource::Initial);
    n_assert(this->resource.isvalid());
    
    // perform direct load    
    if (this->SetupMeshFromMemory())
    {
        this->SetState(Resource::Loaded);
        return true;
    }
    // fallthrough: synchronous loading failed
    this->SetState(Resource::Failed);
    return false;    
}

//------------------------------------------------------------------------------
/**
*/
bool 
MemoryMeshLoader::SetupMeshFromMemory()
{   
    n_assert(this->vertexBuffer.isvalid());

    const Ptr<Mesh>& res = this->resource.downcast<Mesh>();
    res->SetVertexBuffer(this->vertexBuffer);
    if (this->indexBuffer.isvalid())
    {
        res->SetIndexBuffer(this->indexBuffer);    
    }
    if (this->primitiveGroups.Size() > 0)
    {
        res->SetPrimitiveGroups(this->primitiveGroups);
    }   

    return true;
}

} // namespace CoreGraphics
