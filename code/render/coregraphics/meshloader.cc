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
using namespace CoreGraphics;
using namespace Resources;
using namespace Util;
__ImplementClass(CoreGraphics::MeshLoader, 'VKML', Resources::ResourceLoader);

CoreGraphics::VertexLayoutId Layouts[(uint)CoreGraphics::VertexLayoutType::NumTypes];
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
    vlCreateInfo.name = "Normal"_atm;
    vlCreateInfo.comps = {
        VertexComponent{ VertexComponent::IndexName::Position, VertexComponent::Float3, 0 }  
        , VertexComponent{ VertexComponent::IndexName::TexCoord1, VertexComponent::Short2, 0 }
        , VertexComponent{ VertexComponent::IndexName::Normal, VertexComponent::Byte4N, 1 }  
        , VertexComponent{ VertexComponent::IndexName::Tangent, VertexComponent::Byte4N, 1 } 
    };
    Layouts[(uint)CoreGraphics::VertexLayoutType::Normal] = CreateVertexLayout(vlCreateInfo);

    vlCreateInfo.name = "SecondaryUVs"_atm;
    vlCreateInfo.comps = {
        VertexComponent{ VertexComponent::IndexName::Position, VertexComponent::Float3, 0 }  
        , VertexComponent{ VertexComponent::IndexName::TexCoord1, VertexComponent::Short2, 0 }
        , VertexComponent{ VertexComponent::IndexName::Normal, VertexComponent::Byte4N, 1 }  
        , VertexComponent{ VertexComponent::IndexName::Tangent, VertexComponent::Byte4N, 1 }
        , VertexComponent{ VertexComponent::IndexName::TexCoord2, VertexComponent::UShort2N, 1 }
    };
    Layouts[(uint)CoreGraphics::VertexLayoutType::SecondUV] = CreateVertexLayout(vlCreateInfo);

    vlCreateInfo.name = "VertexColors"_atm;
    vlCreateInfo.comps = {
        VertexComponent{ VertexComponent::IndexName::Position, VertexComponent::Float3, 0 }
        , VertexComponent{ VertexComponent::IndexName::TexCoord1, VertexComponent::Short2, 0 }
        , VertexComponent{ VertexComponent::IndexName::Normal, VertexComponent::Byte4N, 1 }
        , VertexComponent{ VertexComponent::IndexName::Tangent, VertexComponent::Byte4N, 1 }
        , VertexComponent{ VertexComponent::IndexName::Color, VertexComponent::Byte4N, 1 }
    };
    Layouts[(uint)CoreGraphics::VertexLayoutType::Colors] = CreateVertexLayout(vlCreateInfo);

    vlCreateInfo.name = "Skin"_atm;
    vlCreateInfo.comps = {
        VertexComponent{ VertexComponent::IndexName::Position, VertexComponent::Float3, 0 }
        , VertexComponent{ VertexComponent::IndexName::TexCoord1, VertexComponent::Short2, 0 }
        , VertexComponent{ VertexComponent::IndexName::Normal, VertexComponent::Byte4N, 1 }
        , VertexComponent{ VertexComponent::IndexName::Tangent, VertexComponent::Byte4N, 1 }
        , VertexComponent{ VertexComponent::IndexName::SkinWeights, VertexComponent::Float4, 1 }
        , VertexComponent{ VertexComponent::IndexName::SkinJIndices, VertexComponent::UByte4, 1 }
    };
    Layouts[(uint)CoreGraphics::VertexLayoutType::Skin] = CreateVertexLayout(vlCreateInfo);

    vlCreateInfo.name = "Particle"_atm;
    vlCreateInfo.comps = {
        VertexComponent(0, CoreGraphics::VertexComponent::Float2, 0)
        , VertexComponent(0, CoreGraphics::VertexComponent::Float4, 1, CoreGraphics::VertexComponent::PerInstance, 1)   // Particle::position
        , VertexComponent(1, CoreGraphics::VertexComponent::Float4, 1, CoreGraphics::VertexComponent::PerInstance, 1)   // Particle::stretchPosition
        , VertexComponent(2, CoreGraphics::VertexComponent::Float4, 1, CoreGraphics::VertexComponent::PerInstance, 1)   // Particle::color
        , VertexComponent(3, CoreGraphics::VertexComponent::Float4, 1, CoreGraphics::VertexComponent::PerInstance, 1)   // Particle::uvMinMax
        , VertexComponent(4, CoreGraphics::VertexComponent::Float4, 1, CoreGraphics::VertexComponent::PerInstance, 1)   // x: Particle::rotation, y: Particle::size
    };
    Layouts[(uint)CoreGraphics::VertexLayoutType::Particle] = CreateVertexLayout(vlCreateInfo);

    CoreGraphics::CmdBufferPoolCreateInfo cmdPoolInfo;
    cmdPoolInfo.queue = CoreGraphics::QueueType::GraphicsQueueType;
    cmdPoolInfo.resetable = false;
    cmdPoolInfo.shortlived = true;
    this->transferPool = CoreGraphics::CreateCmdBufferPool(cmdPoolInfo);

    this->retiredCommandBuffers.Resize(CoreGraphics::GetNumBufferedFrames());
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
MeshLoader::InitializeResource(Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    n_assert(stream.isvalid());
    String resIdExt = this->names[entry].AsString().GetFileExtension();

    MeshResourceId ret = meshResourceAllocator.Alloc();

    if (resIdExt == "nvx")
    {
        this->SetupMeshFromNvx(stream, entry, ret, immediate);
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
uint
MeshLoader::StreamResource(const Resources::ResourceId entry, IndexT frameIndex, uint requestedBits)
{
    ResourceLoader::StreamData& stream = this->streams[entry.loaderInstanceId];
    ResourceName name = this->names[entry.loaderInstanceId];
    MeshStreamData* streamData = (MeshStreamData*)stream.data;
    auto header = (Nvx3Header*)streamData->mappedData;

    n_assert(header->magic == NEBULA_NVX_MAGICNUMBER);
    n_assert(header->numMeshes > 0);
    auto vertexRanges = (Nvx3VertexRange*)(header + 1);
    auto groups = (Nvx3Group*)(vertexRanges + header->numMeshes);
    auto vertexData = (ubyte*)(groups + header->numGroups);
    auto indexData = (ubyte*)(vertexData + header->vertexDataSize);
    //auto meshletData = (Nvx3Meshlet*)(indexData + header->indexDataSize);

    CoreGraphics::BufferId vbo = CoreGraphics::GetVertexBuffer();
    CoreGraphics::BufferId ibo = CoreGraphics::GetIndexBuffer();
    
    int loadBits = 0x0;

    CoreGraphics::CmdBufferCreateInfo cmdCreateInfo;
    cmdCreateInfo.name = name.Value();
    cmdCreateInfo.pool = this->transferPool;
    cmdCreateInfo.usage = CoreGraphics::TransferQueueType;
    cmdCreateInfo.queryTypes = CoreGraphics::CmdBufferQueryBits::NoQueries;
    CoreGraphics::CmdBufferId transferCommands = CoreGraphics::CreateCmdBuffer(cmdCreateInfo);

    CoreGraphics::CmdBufferBeginInfo beginInfo;
    beginInfo.submitOnce = true;
    beginInfo.submitDuringPass = false;
    beginInfo.resubmittable = false;

    CoreGraphics::CmdBeginRecord(transferCommands, beginInfo);
    CoreGraphics::CmdBeginMarker(transferCommands, NEBULA_MARKER_GRAPHICS, name.Value());

    // Upload vertices
    if (requestedBits & ~0x1)
    {
        auto [offset, buffer] = CoreGraphics::UploadArray(vertexData, header->vertexDataSize);
        if (buffer != CoreGraphics::InvalidBufferId)
        {
            BufferIdAcquire(vbo);
            SizeT baseVertexOffset = streamData->vertexAllocationOffset.offset;

            // Copy from host mappable buffer to device local buffer
            CoreGraphics::BufferCopy from, to;
            from.offset = offset;
            to.offset = baseVertexOffset;
            CoreGraphics::CmdCopy(transferCommands, buffer, { from }, vbo, { to }, header->vertexDataSize);
            BufferIdRelease(vbo);

            loadBits |= 1 << 0;
        }
    }

    // Upload indices
    if (requestedBits & ~0x2)
    {
        auto [offset, buffer] = CoreGraphics::UploadArray(indexData, header->indexDataSize);
        if (buffer != CoreGraphics::InvalidBufferId)
        {
            BufferIdAcquire(ibo);
            SizeT baseIndexOffset = streamData->indexAllocationOffset.offset;

            // Copy from host mappable buffer to device local buffer
            CoreGraphics::BufferCopy from, to;
            from.offset = offset;
            to.offset = baseIndexOffset;
            CoreGraphics::CmdCopy(transferCommands, buffer, { from }, ibo, { to }, header->indexDataSize);
            BufferIdRelease(ibo);

            loadBits |= 1 << 1;
        }
    }
    streamData->cmdBuf = transferCommands;

    CoreGraphics::CmdEndMarker(transferCommands);
    CoreGraphics::CmdEndRecord(transferCommands);
    CoreGraphics::CmdBufferIdRelease(transferCommands);

    if (loadBits != 0)
        this->partiallyCompleteResources.Append(entry);

    return loadBits;
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
*/
uint
MeshLoader::LodMask(const Ids::Id32 entry, float lod, bool stream) const
{
    return 0x3;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshLoader::UpdateLoaderSyncState()
{
    Util::Array<CoreGraphics::CmdBufferId>& retiredCommandBuffersThisFrame = this->retiredCommandBuffers[CoreGraphics::GetBufferedFrameIndex()];

    // Cleanup any command buffers from this frame
    for (auto buf : retiredCommandBuffersThisFrame)
    {
        // We own the pool, so we should destroy them directly and not have the graphics device do it
        CoreGraphics::DestroyCmdBuffer(buf);
    }
    retiredCommandBuffersThisFrame.Clear();

    for (auto& entry : this->partiallyCompleteResources)
    {
        ResourceLoader::StreamData& stream = this->streams[entry.loaderInstanceId];
        MeshStreamData* streamData = static_cast<MeshStreamData*>(stream.data);
        CoreGraphics::SubmitCommandBuffers({ streamData->cmdBuf }, CoreGraphics::GraphicsQueueType, "Upload mesh");
        retiredCommandBuffersThisFrame.Append(streamData->cmdBuf);
    }
    this->partiallyCompleteResources.Clear();
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::VertexLayoutId
MeshLoader::GetLayout(const CoreGraphics::VertexLayoutType type)
{
    return Layouts[(uint)type];
}

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from a nvx3 file (Nebula's
    native binary mesh file format).
*/
void
MeshLoader::SetupMeshFromNvx(const Ptr<IO::Stream>& stream, const Ids::Id32 entry, const MeshResourceId meshResource, bool immediate)
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
        //auto meshletData = (Nvx3Meshlet*)(indexData + header->indexDataSize);

        meshes.Resize(header->numMeshes);

        MeshStreamData* streamData = (MeshStreamData*)Memory::Alloc(Memory::ScratchHeap, sizeof(MeshStreamData));
        streamData->mappedData = mapPtr;

        this->streams[entry].stream = stream;
        this->streams[entry].data = streamData;
        streamData->cmdBuf = CoreGraphics::InvalidCmdBufferId;

        CoreGraphics::BufferId vbo = CoreGraphics::GetVertexBuffer();
        CoreGraphics::BufferId ibo = CoreGraphics::GetIndexBuffer();
        CoreGraphics::VertexAlloc vertexAllocation, indexAllocation = { .size = 0xFFFFFFFF, .offset = 0xFFFFFFFF, .node = 0xFFFFFFFF };

        // Upload vertex data
        {
            // Allocate vertices from global repository
            vertexAllocation = CoreGraphics::AllocateVertices(header->vertexDataSize);
            streamData->vertexAllocationOffset = vertexAllocation;

            if (immediate)
            {
                BufferCopyWithStaging(CoreGraphics::GetVertexBuffer(), streamData->vertexAllocationOffset.offset, vertexData, header->vertexDataSize);
            }
        }

        // Upload index data
        {
            // Allocate vertices from global repository
            indexAllocation = CoreGraphics::AllocateIndices(header->indexDataSize);
            streamData->indexAllocationOffset = indexAllocation;

            if (immediate)
            {
                BufferCopyWithStaging(CoreGraphics::GetIndexBuffer(), streamData->indexAllocationOffset.offset, indexData, header->indexDataSize);
            }
        }

        for (uint i = 0; i < header->numGroups; i++)
        {
            PrimitiveGroup group;
            group.SetBaseIndex(groups[i].firstIndex);
            group.SetNumIndices(groups[i].numIndices);
            primGroups.Append(group);
        }

        for (uint i = 0; i < header->numMeshes; i++)
        {
            MeshCreateInfo mshInfo;
            mshInfo.streams.Append({ vbo, (SizeT)(streamData->vertexAllocationOffset.offset + vertexRanges[i].baseVertexByteOffset), 0 });
            mshInfo.streams.Append({ vbo, (SizeT)(streamData->vertexAllocationOffset.offset + vertexRanges[i].attributesVertexByteOffset), 1 });
            mshInfo.indexBufferOffset = streamData->indexAllocationOffset.offset + (SizeT)vertexRanges[i].indexByteOffset;
            mshInfo.indexBuffer = ibo;
            mshInfo.topology = PrimitiveTopology::TriangleList;
            mshInfo.indexType = vertexRanges[i].indexType;
            mshInfo.primitiveGroups = primGroups;
            mshInfo.vertexLayout = Layouts[(uint)vertexRanges[i].layout];
            mshInfo.vertexBufferAllocation = vertexAllocation;
            mshInfo.indexBufferAllocation = indexAllocation;
            mshInfo.name = this->names[entry];
            MeshId mesh = CreateMesh(mshInfo);
            meshes[i] = mesh;
        }

        reader->Close();
    }

    // Update mesh allocator
    meshResourceAllocator.Set<0>(meshResource.id, meshes);
}

} // namespace CoreGraphics

