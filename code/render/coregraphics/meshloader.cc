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
    this->asyncTransferPool = CoreGraphics::CreateCmdBufferPool(cmdPoolInfo);
    this->immediateTransferPool = CoreGraphics::CreateCmdBufferPool(cmdPoolInfo);
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
ResourceLoader::ResourceInitOutput
MeshLoader::InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream)
{
    n_assert(stream.isvalid());
    String resIdExt = job.name.GetFileExtension();

    if (resIdExt == "nvx")
    {
        MeshResourceId id = meshResourceAllocator.Alloc();
        ResourceLoader::ResourceInitOutput ret;

        ret.loaderStreamData = this->SetupMeshFromNvx(stream, job, id);
        ret.id = id;
        return ret;
    }
    else
    {
        n_error("StreamMeshCache::SetupMeshFromStream(): unrecognized file extension in '%s'\n", resIdExt.AsCharPtr());
        return ResourceLoader::ResourceInitOutput();
    }
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::ResourceStreamOutput
MeshLoader::StreamResource(const ResourceLoadJob& job)
{
    const ResourceLoader::_StreamData& stream = job.streamData;
    ResourceName name = job.name;
    MeshStreamData* streamData = (MeshStreamData*)stream.data;
    auto header = (Nvx3Header*)streamData->mappedData;
    char* basePtr = (char*)streamData->mappedData;

    uint loadedBits = job.loadState.loadedBits;
    uint pendingBits = job.loadState.pendingBits;
    uint bitsToLoad = job.loadState.requestedBits & ~(pendingBits | loadedBits);

    n_assert(header->magic == NEBULA_NVX_MAGICNUMBER);
    n_assert(header->numMeshes > 0);
    auto vertexRanges = (Nvx3VertexRange*)(basePtr + header->meshDataOffset);
    auto vertexData = (ubyte*)(basePtr + header->vertexDataOffset);
    auto indexData = (ubyte*)(basePtr + header->indexDataOffset);
    //auto meshletData = (Nvx3Meshlet*)(indexData + header->indexDataSize);

    CoreGraphics::BufferId vbo = CoreGraphics::GetVertexBuffer();
    CoreGraphics::BufferId ibo = CoreGraphics::GetIndexBuffer();

    ResourceLoader::ResourceStreamOutput ret;
    CoreGraphics::CmdBufferId transferCommands;
    Util::Array<Memory::RangeAllocation> rangesToFlush;

    if (bitsToLoad != 0x0)
    {
        CoreGraphics::CmdBufferCreateInfo cmdCreateInfo;
        cmdCreateInfo.name = name.Value();
        cmdCreateInfo.pool = job.immediate ? this->immediateTransferPool : this->asyncTransferPool;
        cmdCreateInfo.usage = CoreGraphics::TransferQueueType;
        cmdCreateInfo.queryTypes = CoreGraphics::CmdBufferQueryBits::NoQueries;
        transferCommands = CoreGraphics::CreateCmdBuffer(cmdCreateInfo);

        CoreGraphics::CmdBufferBeginInfo beginInfo;
        beginInfo.submitOnce = true;
        beginInfo.submitDuringPass = false;
        beginInfo.resubmittable = false;

        CoreGraphics::CmdBeginRecord(transferCommands, beginInfo);
        CoreGraphics::CmdBeginMarker(transferCommands, NEBULA_MARKER_GRAPHICS, name.Value());


        // Upload vertices
        if (bitsToLoad & 0x1)
        {
            auto [alloc, buffer] = CoreGraphics::UploadArray(vertexData, header->vertexDataSize);
            if (buffer != CoreGraphics::InvalidBufferId)
            {
                rangesToFlush.Append(alloc);
                BufferIdAcquire(vbo);
                size_t baseVertexOffset = streamData->vertexAllocationOffset.offset;

                // Copy from host mappable buffer to device local buffer
                CoreGraphics::BufferCopy from, to;
                from.offset = alloc.offset;
                to.offset = baseVertexOffset;
                CoreGraphics::CmdCopy(transferCommands, buffer, { from }, vbo, { to }, header->vertexDataSize);
                BufferIdRelease(vbo);

                pendingBits |= 1 << 0;
            }
        }

        // Upload indices
        if (bitsToLoad & 0x2)
        {
            auto [alloc, buffer] = CoreGraphics::UploadArray(indexData, header->indexDataSize);
            if (buffer != CoreGraphics::InvalidBufferId)
            {
                rangesToFlush.Append(alloc);
                BufferIdAcquire(ibo);
                size_t baseIndexOffset = streamData->indexAllocationOffset.offset;

                // Copy from host mappable buffer to device local buffer
                CoreGraphics::BufferCopy from, to;
                from.offset = alloc.offset;
                to.offset = baseIndexOffset;
                CoreGraphics::CmdCopy(transferCommands, buffer, { from }, ibo, { to }, header->indexDataSize);
                BufferIdRelease(ibo);

                pendingBits |= 1 << 1;
            }
        }

        CoreGraphics::CmdEndMarker(transferCommands);
        CoreGraphics::CmdEndRecord(transferCommands);
        CoreGraphics::CmdBufferIdRelease(transferCommands);

        if (pendingBits != 0)
        {
            if (job.immediate)
            {
                CoreGraphics::FlushUploads(rangesToFlush);
                CoreGraphics::SubmissionWaitEvent waitEvent = CoreGraphics::SubmitCommandBuffers({ transferCommands }, CoreGraphics::GraphicsQueueType, nullptr, "Upload meshes");
                CoreGraphics::DeferredDestroyCmdBuffer(transferCommands);

                IndexT index = this->meshesToFinish.FindIndex(job.id);
                if (index == InvalidIndex)
                    this->meshesToFinish.Add(job.id, { FinishedMesh{ .submissionId = waitEvent.timelineIndex, .bits = pendingBits, .rangesToFree = rangesToFlush, .cmdBuf = transferCommands } });
                else
                    this->meshesToFinish.ValueAtIndex(job.id, index).Append(FinishedMesh{ .submissionId = waitEvent.timelineIndex, .bits = ret.pendingBits, .rangesToFree = rangesToFlush, .cmdBuf = transferCommands });
            }
            else
            {
                this->meshesToSubmit.Enqueue(MeshesToSubmit{ .id = job.id, .bits = pendingBits, .rangesToFlush = rangesToFlush, .cmdBuf = transferCommands });
            }
        }
    }

    if (job.loadState.pendingBits != 0x0)
    {
        this->meshLock.Enter();
        IndexT index = this->meshesToFinish.FindIndex(job.id);
        if (index != InvalidIndex)
        {
            Util::Array<FinishedMesh>& meshes = this->meshesToFinish.ValueAtIndex(job.id, index);
            for (int i = 0; i < meshes.Size(); i++)
            {
                const FinishedMesh& mesh = meshes[i];
                if (CoreGraphics::PollSubmissionIndex(CoreGraphics::GraphicsQueueType, mesh.submissionId))
                {
                    CoreGraphics::FreeUploads(mesh.rangesToFree);
                    CoreGraphics::DestroyCmdBuffer(mesh.cmdBuf);
                    loadedBits |= mesh.bits;
                    pendingBits &= ~mesh.bits;
                    meshes.EraseIndex(i);
                    i--;
                }
            }
            if (meshes.IsEmpty())
                this->meshesToFinish.EraseIndex(job.id, index);
        }
        this->meshLock.Leave();
    }
    else if (job.loadState.pendingBits == 0x0 && bitsToLoad == 0x0)
    {
        n_warning("Resource '%s' is stuck in an infinite state", job.name.AsCharPtr());
    }

    ret.loadedBits = loadedBits;
    ret.pendingBits = pendingBits;
    return ret;
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
MeshLoader::LodMask(const _StreamData& stream, float lod, bool async) const
{
    return 0x3;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshLoader::UpdateLoaderSyncState()
{
    Util::Array<MeshesToSubmit, 128> meshesToSubmit;
    this->meshesToSubmit.DequeueAll(meshesToSubmit);

    // Enqueue cleanups
    for (const auto& submit : meshesToSubmit)
    {
        CoreGraphics::FlushUploads(submit.rangesToFlush);
        CoreGraphics::SubmissionWaitEvent waitEvent = CoreGraphics::SubmitCommandBuffers({ submit.cmdBuf }, CoreGraphics::GraphicsQueueType, nullptr, "Upload meshes");

        this->meshLock.Enter();
        IndexT index = meshesToFinish.FindIndex(submit.id);
        if (index == InvalidIndex)
            this->meshesToFinish.Add(submit.id, { FinishedMesh{ .submissionId = waitEvent.timelineIndex, .bits = submit.bits, .rangesToFree = submit.rangesToFlush, .cmdBuf = submit.cmdBuf } });
        else
            this->meshesToFinish.ValueAtIndex(submit.id, index).Append(FinishedMesh{ .submissionId = waitEvent.timelineIndex, .bits = submit.bits, .rangesToFree = submit.rangesToFlush, .cmdBuf = submit.cmdBuf });
        this->meshLock.Leave();
    }
    meshesToSubmit.Clear();
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
ResourceLoader::_StreamData
MeshLoader::SetupMeshFromNvx(const Ptr<IO::Stream>& stream, const ResourceLoadJob& job, const MeshResourceId meshResource)
{
    n_assert(stream.isvalid());

    void* mapPtr = nullptr;
    Util::FixedArray<MeshId> meshes;

    ResourceLoader::_StreamData ret;

    Ptr<IO::StreamReader> reader = IO::StreamReader::Create();
    reader->SetStream(stream);
    if (reader->Open())
    {
        n_assert(stream->CanBeMapped());
        n_assert(nullptr == mapPtr);

        // map the stream to memory
        mapPtr = stream->MemoryMap();
        char* basePtr = (char*)mapPtr;

        n_assert(nullptr != mapPtr);

        auto header = (Nvx3Header*)mapPtr;
        if (header->magic != NEBULA_NVX_MAGICNUMBER)
        {
            // not a nvx2 file, break hard
            n_error("MeshLoader: '%s' is not a nvx file!", stream->GetURI().AsString().AsCharPtr());
        }

        n_assert(header->numMeshes > 0);
        auto vertexRanges = (Nvx3VertexRange*)(basePtr + header->meshDataOffset);
        auto vertexData = (ubyte*)(basePtr + header->vertexDataOffset);
        auto indexData = (ubyte*)(basePtr + header->indexDataOffset);
        //auto meshletData = (Nvx3Meshlet*)(indexData + header->indexDataSize);

        meshes.Resize(header->numMeshes);

        MeshStreamData* streamData = (MeshStreamData*)Memory::Alloc(Memory::ScratchHeap, sizeof(MeshStreamData));
        streamData->mappedData = mapPtr;

        ret.stream = stream;
        ret.data = streamData;

        CoreGraphics::BufferId vbo = CoreGraphics::GetVertexBuffer();
        CoreGraphics::BufferId ibo = CoreGraphics::GetIndexBuffer();
        CoreGraphics::VertexAlloc vertexAllocation, indexAllocation = { .size = 0xFFFFFFFF, .offset = 0xFFFFFFFF, .node = 0xFFFFFFFF };

        n_assert(header->vertexDataSize > 0);
        n_assert(header->indexDataSize > 0);
        // Upload vertex data
        {
            // Allocate vertices from global repository
            vertexAllocation = CoreGraphics::AllocateVertices(header->vertexDataSize);
            streamData->vertexAllocationOffset = vertexAllocation;
            meshResourceAllocator.Set<MeshResource_VertexData>(meshResource.id, vertexAllocation);
            if (job.immediate)
            {
                BufferCopyWithStaging(CoreGraphics::GetVertexBuffer(), streamData->vertexAllocationOffset.offset, vertexData, header->vertexDataSize);
            }
        }

        // Upload index data
        {
            // Allocate vertices from global repository
            indexAllocation = CoreGraphics::AllocateIndices(header->indexDataSize);
            streamData->indexAllocationOffset = indexAllocation;
            meshResourceAllocator.Set<MeshResource_IndexData>(meshResource.id, indexAllocation);
            if (job.immediate)
            {
                BufferCopyWithStaging(CoreGraphics::GetIndexBuffer(), streamData->indexAllocationOffset.offset, indexData, header->indexDataSize);
            }
        }

        for (uint i = 0; i < header->numMeshes; i++)
        {
            Util::Array<CoreGraphics::PrimitiveGroup> primGroups;
            const Nvx3VertexRange& range = vertexRanges[i];

            for (uint j = 0; j < range.numGroups; j++)
            {
                PrimitiveGroup group;
                const Nvx3Group* nvxGroup = (Nvx3Group*)(basePtr + range.firstGroupOffset + j * sizeof(Nvx3Group));
                group.SetBaseIndex(nvxGroup->firstIndex);
                group.SetNumIndices(nvxGroup->numIndices);
                primGroups.Append(group);
            }
            MeshCreateInfo mshInfo;
            mshInfo.streams.Append({ vbo, (streamData->vertexAllocationOffset.offset + (size_t)range.baseVertexByteOffset), 0 });
            mshInfo.streams.Append({ vbo, (streamData->vertexAllocationOffset.offset + (size_t)range.attributesVertexByteOffset), 1 });
            mshInfo.indexBufferOffset = streamData->indexAllocationOffset.offset + (size_t)range.indexByteOffset;
            mshInfo.indexBuffer = ibo;
            mshInfo.topology = PrimitiveTopology::TriangleList;
            mshInfo.indexType = range.indexType;
            mshInfo.primitiveGroups = primGroups;
            mshInfo.vertexLayout = Layouts[(uint)range.layout];
            mshInfo.name = job.name;
            MeshId mesh = CreateMesh(mshInfo);
            meshes[i] = mesh;
        }

        reader->Close();
    }

    // Update mesh allocator
    meshResourceAllocator.Set<MeshResource_Meshes>(meshResource.id, meshes);

    return ret;
}

} // namespace CoreGraphics

