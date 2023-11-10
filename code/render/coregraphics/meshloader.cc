//------------------------------------------------------------------------------
// vkstreammeshloader.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "meshloader.h"
#include "coregraphics/mesh.h"
#include "nvx3fileformatstructs.h"
#include "coregraphics/meshloader.h"
#include "coregraphics/graphicsdevice.h"

namespace CoreGraphics
{
using namespace IO;
using namespace CoreGraphics;
using namespace Util;
__ImplementClass(CoreGraphics::MeshLoader, 'VKML', Resources::ResourceLoader);

CoreGraphics::VertexLayoutId layouts[(uint)CoreGraphics::VertexLayoutType::NumTypes];
//------------------------------------------------------------------------------
/**
*/
MeshLoader::MeshLoader()
{
    this->placeholderResourceName = "sysmsh:placeholder.nvx";
    this->failResourceName = "sysmsh:error.nvx";
    this->async = true;

    this->streamerThreadName = "Mesh Streamer Thread";

    // Setup vertex layouts
    CoreGraphics::VertexLayoutCreateInfo vlCreateInfo;
    vlCreateInfo.comps = {
        VertexComponent{ VertexComponent::IndexName::Position, VertexComponent::Float3, 0 }  
        , VertexComponent{ VertexComponent::IndexName::TexCoord1, VertexComponent::Short2, 0 }
        , VertexComponent{ VertexComponent::IndexName::Normal, VertexComponent::Byte4N, 1 }  
        , VertexComponent{ VertexComponent::IndexName::Tangent, VertexComponent::Byte4N, 1 } 
    };
    layouts[(uint)CoreGraphics::VertexLayoutType::Normal] = CreateVertexLayout(vlCreateInfo);

    vlCreateInfo.comps = {
        VertexComponent{ VertexComponent::IndexName::Position, VertexComponent::Float3, 0 }  
        , VertexComponent{ VertexComponent::IndexName::TexCoord1, VertexComponent::Short2, 0 }
        , VertexComponent{ VertexComponent::IndexName::Normal, VertexComponent::Byte4N, 1 }  
        , VertexComponent{ VertexComponent::IndexName::Tangent, VertexComponent::Byte4N, 1 }
        , VertexComponent{ VertexComponent::IndexName::TexCoord2, VertexComponent::UShort2N, 1 }
    };
    layouts[(uint)CoreGraphics::VertexLayoutType::SecondUV] = CreateVertexLayout(vlCreateInfo);

    vlCreateInfo.comps = {
        VertexComponent{ VertexComponent::IndexName::Position, VertexComponent::Float3, 0 }
        , VertexComponent{ VertexComponent::IndexName::TexCoord1, VertexComponent::Short2, 0 }
        , VertexComponent{ VertexComponent::IndexName::Normal, VertexComponent::Byte4N, 1 }
        , VertexComponent{ VertexComponent::IndexName::Tangent, VertexComponent::Byte4N, 1 }
        , VertexComponent{ VertexComponent::IndexName::Color, VertexComponent::Byte4N, 1 }
    };
    layouts[(uint)CoreGraphics::VertexLayoutType::Colors] = CreateVertexLayout(vlCreateInfo);

    vlCreateInfo.comps = {
        VertexComponent{ VertexComponent::IndexName::Position, VertexComponent::Float3, 0 }
        , VertexComponent{ VertexComponent::IndexName::TexCoord1, VertexComponent::Short2, 0 }
        , VertexComponent{ VertexComponent::IndexName::Normal, VertexComponent::Byte4N, 1 }
        , VertexComponent{ VertexComponent::IndexName::Tangent, VertexComponent::Byte4N, 1 }
        , VertexComponent{ VertexComponent::IndexName::SkinWeights, VertexComponent::Float4, 1 }
        , VertexComponent{ VertexComponent::IndexName::SkinJIndices, VertexComponent::UByte4, 1 }
    };
    layouts[(uint)CoreGraphics::VertexLayoutType::Skin] = CreateVertexLayout(vlCreateInfo);
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

    MeshResourceId ret = { meshResourceAllocator.Alloc(), MeshIdType };

    if (resIdExt == "nvx")
    {
        this->SetupMeshFromNvx(stream, ret);
        return ret;
    }
    else
    {
        n_error("StreamMeshCache::SetupMeshFromStream(): unrecognized file extension in '%s'\n", resIdExt.AsCharPtr());
        return InvalidMeshResourceId;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MeshLoader::Unload(const Resources::ResourceId id)
{
    DestroyMeshResource(id);
}

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from a nvx3 file (Nebula's
    native binary mesh file format).
*/
void
MeshLoader::SetupMeshFromNvx(const Ptr<IO::Stream>& stream, const MeshResourceId entry)
{
    n_assert(stream.isvalid());

    Util::Array<CoreGraphics::PrimitiveGroup> primGroups;
    void* mapPtr = nullptr;
    Util::FixedArray<MeshId> meshes;

    Ptr<IO::StreamReader> reader = IO::StreamReader::Create();
    reader->SetStream(stream);
    if (reader->Open())
    {
        n_assert(0 == primGroups.Size());
        n_assert(stream->CanBeMapped());
        n_assert(nullptr == mapPtr);

        // map the stream to memory
        mapPtr = stream->MemoryMap();

        n_assert(nullptr != mapPtr);

        auto header = (Nvx3Header*)mapPtr;
        if (header->magic != NEBULA_NVX_MAGICNUMBER)
        {
            // not a nvx2 file, break hard
            n_error("MeshLoader: '%s' is not a nvx file!", stream->GetURI().AsString().AsCharPtr());
        }

        n_assert(header->numMeshes > 0);
        auto vertexRanges = (Nvx3VertexRange*)(header + 1);
        auto groups = (Nvx3Group*)(vertexRanges + header->numMeshes);
        auto vertexData = (ubyte*)(groups + header->numGroups);
        auto indexData = (ubyte*)(vertexData + header->vertexDataSize);
        auto meshletData = (Nvx3Meshlet*)(indexData + header->indexDataSize);

        meshes.Resize(header->numMeshes);

        SizeT baseVertexOffset, baseIndexOffset;
        CoreGraphics::BufferId vbo = CoreGraphics::GetVertexBuffer();
        CoreGraphics::BufferId ibo = CoreGraphics::GetIndexBuffer();
        CoreGraphics::VertexAlloc vertexAllocation, indexAllocation = { .size = 0xFFFFFFFF, .offset = 0xFFFFFFFF, .node = 0xFFFFFFFF };

        // Upload vertex data
        {
            BufferIdAcquire(vbo);

            // Get upload buffer
            auto [offset, buffer] = CoreGraphics::UploadArray(vertexData, header->vertexDataSize);

            // Allocate vertices from global repository
            vertexAllocation = CoreGraphics::AllocateVertices(header->vertexDataSize);
            baseVertexOffset = vertexAllocation.offset;

            // Copy from host mappable buffer to device local buffer
            CoreGraphics::BufferCopy from, to;
            from.offset = offset;
            to.offset = baseVertexOffset;
            CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer();
            CoreGraphics::CmdCopy(cmdBuf, buffer, {from}, vbo, {to}, header->vertexDataSize);
            CoreGraphics::UnlockGraphicsSetupCommandBuffer();

            BufferIdRelease(vbo);
        }

        // Upload index data
        {
            BufferIdAcquire(ibo);

            // Get upload buffer
            auto [offset, buffer] = CoreGraphics::UploadArray(indexData, header->indexDataSize);

            // Allocate vertices from global repository
            indexAllocation = CoreGraphics::AllocateIndices(header->indexDataSize);
            baseIndexOffset = indexAllocation.offset;

            // Copy from host mappable buffer to device local buffer
            CoreGraphics::BufferCopy from, to;
            from.offset = offset;
            to.offset = baseIndexOffset;
            CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer();
            CoreGraphics::CmdCopy(cmdBuf, buffer, {from}, ibo, {to}, header->indexDataSize);
            CoreGraphics::UnlockGraphicsSetupCommandBuffer();

            BufferIdRelease(ibo);
        }

        for (IndexT i = 0; i < header->numGroups; i++)
        {
            PrimitiveGroup group;
            group.SetBaseIndex(groups[i].firstIndex);
            group.SetNumIndices(groups[i].numIndices);
            primGroups.Append(group);
        }

        for (IndexT i = 0; i < header->numMeshes; i++)
        {
            MeshCreateInfo mshInfo;
            mshInfo.streams.Append({ vbo, baseVertexOffset + (SizeT)vertexRanges[i].baseVertexByteOffset, 0 });
            mshInfo.streams.Append({ vbo, baseVertexOffset + (SizeT)vertexRanges[i].attributesVertexByteOffset, 1 });
            mshInfo.indexBufferOffset = baseIndexOffset + (SizeT)vertexRanges[i].indexByteOffset;
            mshInfo.indexBuffer = ibo;
            mshInfo.topology = PrimitiveTopology::TriangleList;
            mshInfo.indexType = vertexRanges[i].indexType;
            mshInfo.primitiveGroups = primGroups;
            mshInfo.vertexLayout = layouts[(uint)vertexRanges[i].layout];
            mshInfo.vertexBufferAllocation = vertexAllocation;
            mshInfo.indexBufferAllocation = indexAllocation;
            MeshId mesh = CreateMesh(mshInfo);
            meshes[i] = mesh;
        }

        reader->Close();
    }

    // Update mesh allocator
    meshResourceAllocator.Set<0>(entry.resourceId, meshes);
}

} // namespace CoreGraphics

