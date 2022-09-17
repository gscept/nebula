//------------------------------------------------------------------------------
// vkstreammeshloader.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "meshloader.h"
#include "coregraphics/mesh.h"
#include "coregraphics/legacy/nvx2streamreader.h"
#include "coregraphics/meshloader.h"
#include "coregraphics/graphicsdevice.h"

namespace CoreGraphics
{
using namespace IO;
using namespace CoreGraphics;
using namespace Util;
__ImplementClass(CoreGraphics::MeshLoader, 'VKML', Resources::ResourceLoader);
//------------------------------------------------------------------------------
/**
*/
MeshLoader::MeshLoader() :
    activeMesh(Ids::InvalidId24)
{
    this->placeholderResourceName = "msh:system/placeholder.nvx2";
    this->failResourceName = "msh:system/error.nvx2";
    this->async = true;

    this->streamerThreadName = "Mesh Pool Streamer Thread";
}

//------------------------------------------------------------------------------
/**
*/
MeshLoader::~MeshLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceUnknownId
MeshLoader::LoadFromStream(Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    n_assert(stream.isvalid());
    String resIdExt = this->names[entry].AsString().GetFileExtension();

#if NEBULA_LEGACY_SUPPORT
    if (resIdExt == "nvx2")
    {
        return this->SetupMeshFromNvx2(stream, entry);
    }
    else
#endif
    if (resIdExt == "nvx3")
    {
        return this->SetupMeshFromNvx3(stream, entry);
    }
    else if (resIdExt == "n3d3")
    {
        return this->SetupMeshFromN3d3(stream, entry);
    }
    else
    {
        n_error("StreamMeshCache::SetupMeshFromStream(): unrecognized file extension in '%s'\n", resIdExt.AsCharPtr());
        return InvalidMeshId;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MeshLoader::Unload(const Resources::ResourceId id)
{
    MeshId mesh;
    mesh.resourceId = id.resourceId;
    mesh.resourceType = id.resourceType;
    DestroyMesh(mesh);
}

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from legacy nvx2 file (Nebula2 binary mesh format).
*/
#if NEBULA_LEGACY_SUPPORT
MeshId
MeshLoader::SetupMeshFromNvx2(const Ptr<IO::Stream>& stream, const Ids::Id32 entry)
{
    n_assert(stream.isvalid());

    Ptr<Legacy::Nvx2StreamReader> nvx2Reader = Legacy::Nvx2StreamReader::Create();
    nvx2Reader->SetStream(stream);
    nvx2Reader->SetUsage(this->usage);
    nvx2Reader->SetAccess(this->access);
    Resources::ResourceName name = this->names[entry];

    // get potential metadata
    const _LoadMetaData& metaData = this->metaData[entry];
    if (metaData.data != nullptr)
    {
        StreamMeshLoadMetaData* typedMetadata = static_cast<StreamMeshLoadMetaData*>(metaData.data);
        nvx2Reader->SetBuffersCopySource(typedMetadata->copySource);
    }

    // opening the reader also loads the file
    if (nvx2Reader->Open(name))
    {
        n_assert(this->states[entry] == Resources::Resource::Pending);
        auto vertexLayout = CreateVertexLayout({ nvx2Reader->GetVertexComponents() });

        MeshCreateInfo mshInfo;
        mshInfo.streams.Append({ nvx2Reader->GetVertexBuffer(), nvx2Reader->GetBaseVertexOffset(), 0 });
        mshInfo.indexBufferOffset = nvx2Reader->GetBaseIndexOffset();
        mshInfo.indexBuffer = nvx2Reader->GetIndexBuffer();
        mshInfo.topology = PrimitiveTopology::TriangleList;
        mshInfo.primitiveGroups = nvx2Reader->GetPrimitiveGroups();
        mshInfo.vertexLayout = vertexLayout;

        // nvx2 does not have per primitive layouts, we apply them to all
        for (auto& i : mshInfo.primitiveGroups)
        {
            i.SetVertexLayout(vertexLayout);
        }
        MeshId mesh = CreateMesh(mshInfo);

        nvx2Reader->Close();
        return mesh;
    }
    return InvalidMeshId;
}
#endif

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from a nvx3 file (Nebula's
    native binary mesh file format).
*/
MeshId
MeshLoader::SetupMeshFromNvx3(const Ptr<IO::Stream>& stream, const Ids::Id32 entry)
{
    // FIXME!
    n_error("StreamMeshCache::SetupMeshFromNvx3() not yet implemented");
    return InvalidMeshId;
}

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from a n3d3 file (Nebula's
    native ascii mesh file format).
*/
MeshId
MeshLoader::SetupMeshFromN3d3(const Ptr<IO::Stream>& stream, const Ids::Id32 entry)
{
    // FIXME!
    n_error("StreamMeshCache::SetupMeshFromN3d3() not yet implemented");
    return InvalidMeshId;
}

} // namespace Vulkan
