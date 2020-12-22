//------------------------------------------------------------------------------
// vkstreammeshloader.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkmeshpool.h"
#include "coregraphics/mesh.h"
#include "coregraphics/legacy/nvx2streamreader.h"

namespace Vulkan
{
using namespace IO;
using namespace CoreGraphics;
using namespace Util;
__ImplementClass(Vulkan::VkMeshPool, 'VKML', Resources::ResourceStreamPool);
//------------------------------------------------------------------------------
/**
*/
VkMeshPool::VkMeshPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
VkMeshPool::~VkMeshPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::LoadStatus
VkMeshPool::Load(const Ptr<Resources::Resource>& res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream)
{
    n_assert(stream.isvalid());
    n_assert(res.isvalid());
    String resIdExt = res->GetResourceName().AsString().GetFileExtension();

#if NEBULA_LEGACY_SUPPORT
    if (resIdExt == "nvx2")
    {
        return this->SetupMeshFromNvx2(stream);
    }
    else
#endif

        if (resIdExt == "nvx3")
        {
            return this->SetupMeshFromNvx3(stream);
        }
        else if (resIdExt == "n3d3")
        {
            return this->SetupMeshFromN3d3(stream);
        }
        else
        {
            n_error("OGL4StreamMeshLoader::SetupMeshFromStream(): unrecognized file extension in '%s'\n", this->resource->GetResourceId().Value());
            return false;
        }
}

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from legacy nvx2 file (Nebula2 binary mesh format).
*/
#if NEBULA_LEGACY_SUPPORT
bool
VkMeshPool::SetupMeshFromNvx2(const Ptr<Stream>& stream, const Ptr<Resources::Resource>& res)
{
    n_assert(stream.isvalid());
    Ptr<Legacy::Nvx2StreamReader> nvx2Reader = Legacy::Nvx2StreamReader::Create();
    nvx2Reader->SetStream(stream);
    nvx2Reader->SetUsage(this->usage);
    nvx2Reader->SetAccess(this->access);
    if (nvx2Reader->Open())
    {
        const Ptr<Mesh>& res = res.downcast<Mesh>();
        n_assert(!res->IsLoaded());
        res->SetVertexBuffer(nvx2Reader->GetVertexBuffer().downcast<CoreGraphics::VertexBuffer>());
        res->SetIndexBuffer(nvx2Reader->GetIndexBuffer().downcast<CoreGraphics::IndexBuffer>());
        res->SetPrimitiveGroups(nvx2Reader->GetPrimitiveGroups());
        res->SetTopology(PrimitiveTopology::TriangleList);
        nvx2Reader->Close();
        return true;
    }
    return false;
}
#endif

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from a nvx3 file (Nebula's
    native binary mesh file format).
*/
bool
VkMeshPool::SetupMeshFromNvx3(const Ptr<Stream>& stream, const Ptr<Resources::Resource>& res)
{
    // FIXME!
    n_error("VkMeshPool::SetupMeshFromNvx3() not yet implemented");
    return false;
}

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from a n3d3 file (Nebula's
    native ascii mesh file format).
*/
bool
VkMeshPool::SetupMeshFromN3d3(const Ptr<Stream>& stream, const Ptr<Resources::Resource>& res)
{
    // FIXME!
    n_error("VkMeshPool::SetupMeshFromN3d3() not yet implemented");
    return false;
}

} // namespace Vulkan