//------------------------------------------------------------------------------
//  textureloader.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "coregraphics/textureloader.h"
#include "coregraphics/load/glimltypes.h"
#include "util/bit.h"

namespace CoreGraphics
{

__ImplementClass(CoreGraphics::TextureLoader, 'TXLO', Resources::ResourceLoader);

struct TextureStreamData
{
    gliml::context ctx;
    void* mappedBuffer;
    uint mappedBufferSize;

    ubyte numMips;
    ubyte nextLayerToLoad;
    ubyte numLayersToLoad, numLayers;
    uchar layers[15];
};

using namespace CoreGraphics;
using namespace Resources;
using namespace IO;


//------------------------------------------------------------------------------
/**
    Attempt to upload texture data to GPU memory
    May fail if the upload buffer is full, in which case the function returns false
*/
bool
UploadToTexture(const CoreGraphics::TextureId texture, const CoreGraphics::CmdBufferId cmdBuf, gliml::context& ctx, uchar layer, uint mip)
{
    // Attempt to upload
    CoreGraphics::TextureSubresourceInfo subres(CoreGraphics::ImageBits::ColorBits, mip, 1, layer, 1);

    CoreGraphics::PixelFormat::Code fmt = TextureGetPixelFormat(texture);
    uint blockSize = CoreGraphics::PixelFormat::ToBlockSize(fmt);
    SizeT alignment = CoreGraphics::PixelFormat::ToTexelSize(fmt) / blockSize;
    auto [offset, buffer] = CoreGraphics::UploadArray((byte*)ctx.image_data(layer, mip), ctx.image_size(layer, mip), alignment);
    if (buffer == CoreGraphics::InvalidBufferId)
    {
        return false;
    }
    else
    {
        // If upload is successful, put a barrier on the buffer
        CoreGraphics::CmdBarrier(
            cmdBuf
            , CoreGraphics::PipelineStage::ImageInitial
            , CoreGraphics::PipelineStage::TransferWrite
            , CoreGraphics::BarrierDomain::Global
            , { TextureBarrierInfo{ .tex = texture, .subres = subres } }
        );

        // Then run a copy on the command buffer
        uint width = ctx.image_width(layer, mip);
        uint height = ctx.image_height(layer, mip);
        CoreGraphics::BufferCopy bufCopy;
        bufCopy.offset = offset;
        bufCopy.imageHeight = 0;
        bufCopy.rowLength = 0;
        CoreGraphics::TextureCopy texCopy;
        texCopy.layer = layer;
        texCopy.mip = mip;
        texCopy.region.set(0, 0, width, height);
        CoreGraphics::CmdCopy(cmdBuf, buffer, { bufCopy }, texture, { texCopy });
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
uint
LoadMips(CoreGraphics::CmdBufferId cmdBuf, TextureStreamData* streamData, uint bitsToLoad, const CoreGraphics::TextureId texture)
{
    // use resource submission
    uint loadedBits = 0x0;
    while (bitsToLoad != 0x0)
    {
        uint mipIndexToLoad = Util::FirstOne(bitsToLoad);
        uint mipToLoad = streamData->numMips - 1 - mipIndexToLoad;

        uint layerMask = streamData->layers[mipToLoad];
        uint numLayers = Util::PopCnt(layerMask);
        while (layerMask != 0x0)
        {
            uint layer = Util::FirstOne(layerMask);

            // Attempt to upload, if it fails we continue from here next time
            if (!UploadToTexture(texture, cmdBuf, streamData->ctx, layer, mipToLoad))
            {
                // If upload fails, escape the loop
                goto quit_loop;
            }

            layerMask &= ~(1 << layer);
        }
        loadedBits |= (1 << mipIndexToLoad);
        bitsToLoad &= ~(1 << mipIndexToLoad);
    }

quit_loop:

    return loadedBits;
}

//------------------------------------------------------------------------------
/**
*/
void
FinishMips(CoreGraphics::CmdBufferId transferCommands, CoreGraphics::CmdBufferId handoverCommands, TextureStreamData* streamData, uint mipBits, const CoreGraphics::TextureId texture, const char* name)
{
    // Finish the mips by handing them over 
    Util::FixedArray<TextureBarrierInfo> barriers(Util::PopCnt(mipBits) * streamData->numLayers);

    uint mipIndexToLoad = Util::FirstOne(mipBits);
    uint barrierCounter = 0;
    while (mipBits != 0x0)
    {
        uint mipIndexToLoad = Util::FirstOne(mipBits);
        uint mipToLoad = streamData->numMips - 1 - mipIndexToLoad;

        for (uint layer = 0; layer < streamData->numLayers; layer++)
        {
            CoreGraphics::TextureSubresourceInfo subres(CoreGraphics::ImageBits::ColorBits, mipToLoad, 1, layer, 1);
            barriers[barrierCounter++] = TextureBarrierInfo{ .tex = texture, .subres = subres };
        }
        mipBits &= ~(1 << mipIndexToLoad);
    }

    if (barrierCounter > 0)
    {
        // Issue handover
        CoreGraphics::CmdHandover(
            transferCommands
            , handoverCommands
            , CoreGraphics::PipelineStage::TransferWrite
            , CoreGraphics::PipelineStage::AllShadersRead
            , barriers
            , nullptr
            , CoreGraphics::GetQueueIndex(QueueType::TransferQueueType)
            , CoreGraphics::GetQueueIndex(QueueType::GraphicsQueueType)
        );
    }
}

//------------------------------------------------------------------------------
/**
*/
TextureLoader::TextureLoader()
{
    this->async = true;
    this->placeholderResourceName = "systex:white.dds";
    this->failResourceName = "systex:error.dds";

    this->streamerThreadName = "Texture Streamer Thread";

    CoreGraphics::CmdBufferPoolCreateInfo cmdPoolInfo;
    cmdPoolInfo.queue = CoreGraphics::QueueType::TransferQueueType;
    cmdPoolInfo.resetable = false;
    cmdPoolInfo.shortlived = true;
    this->asyncTransferPool = CoreGraphics::CreateCmdBufferPool(cmdPoolInfo);
    this->immediateTransferPool = CoreGraphics::CreateCmdBufferPool(cmdPoolInfo);
    cmdPoolInfo.queue = CoreGraphics::QueueType::GraphicsQueueType;
    this->asyncHandoverPool = CoreGraphics::CreateCmdBufferPool(cmdPoolInfo);
    this->immediateHandoverPool = CoreGraphics::CreateCmdBufferPool(cmdPoolInfo);
}

//------------------------------------------------------------------------------
/**
*/
TextureLoader::~TextureLoader()
{
    // empty
    CoreGraphics::DestroyCmdBufferPool(this->asyncTransferPool);
    CoreGraphics::DestroyCmdBufferPool(this->immediateTransferPool);
    CoreGraphics::DestroyCmdBufferPool(this->asyncHandoverPool);
    CoreGraphics::DestroyCmdBufferPool(this->immediateHandoverPool);
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::ResourceInitOutput
TextureLoader::InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream)
{
    N_SCOPE_ACCUM(CreateAndLoad, TextureStream);
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());

    // Map memory, we will keep the memory mapping so we can stream in LODs later
    void* srcData = stream->MemoryMap();
    uint srcDataSize = stream->GetSize();

    ResourceLoader::ResourceInitOutput ret;
    ret.id = InvalidResourceUnknownId;
    ret.loaderStreamData = _StreamData{ .stream = nullptr, .data = nullptr };

    // load using gliml
    gliml::context ctx;
    if (ctx.load_dds(srcData, srcDataSize))
    {
        // We have a total amount of mip maps, say 10
        int numMips = ctx.num_mipmaps(0);
        int depth = ctx.image_depth(0, 0);
        int width = ctx.image_width(0, 0);
        int height = ctx.image_height(0, 0);
        int layers = ctx.num_faces();

        auto streamData = (TextureStreamData*)Memory::Alloc(Memory::ScratchHeap, sizeof(TextureStreamData));
        memset(streamData, 0x0, sizeof(TextureStreamData));

        streamData->mappedBuffer = srcData;
        streamData->mappedBufferSize = srcDataSize;
        streamData->ctx = ctx;
        streamData->numMips = numMips;
        streamData->numLayers = layers;
        streamData->nextLayerToLoad = 0;
        streamData->numLayersToLoad = layers;
        memset(streamData->layers, 0x0, sizeof(streamData->layers));
        for (uint i = 0; i < numMips; i++)
        {
            streamData->layers[i] = (1 << layers) - 1;
        }

        ret.loaderStreamData = _StreamData{ .stream = stream, .data = streamData };

        CoreGraphics::PixelFormat::Code format = CoreGraphics::Gliml::ToPixelFormat(ctx);
        CoreGraphics::TextureType type = ctx.is_3d() ? CoreGraphics::Texture3D : (layers == 6 ? CoreGraphics::TextureCube : CoreGraphics::Texture2D);

        CoreGraphics::TextureCreateInfo textureInfo;
        textureInfo.name = job.name.AsCharPtr();
        textureInfo.width = width;
        textureInfo.height = height;
        textureInfo.depth = depth;
        textureInfo.mips = numMips;
        textureInfo.minMip = numMips - 1;
        textureInfo.layers = layers;
        textureInfo.type = type;
        textureInfo.format = format;
        if (job.immediate)
        {
            textureInfo.minMip = 0;
            textureInfo.data = ctx.image_data(0, 0);
            for (IndexT i = 0; i < ctx.num_faces(); i++)
            {
                for (IndexT j = 0; j < ctx.num_mipmaps(i); j++)
                {
                    textureInfo.dataSize += ctx.image_size(i, j);
                }
            }
        }
            
        CoreGraphics::TextureId texture = CoreGraphics::CreateTexture(textureInfo);

        TextureIdRelease(texture);
        ret.id = texture;
        return ret;
    }

    stream->MemoryUnmap();
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::ResourceStreamOutput
TextureLoader::StreamResource(const ResourceLoadJob& job)
{
    // Get resource data
    TextureStreamData* streamData = static_cast<TextureStreamData*>(job.streamData.data);
    ResourceName name = job.name;

    uint loadedBits = job.loadState.loadedBits;
    uint pendingBits = job.loadState.pendingBits;
    uint bitsToLoad = job.loadState.requestedBits & ~(pendingBits | loadedBits);

    ResourceLoader::ResourceStreamOutput ret;


    // Setup texture id
    TextureId texture = job.id;
    TextureIdAcquire(texture);

    if (bitsToLoad != 0x0)
    {
        CoreGraphics::CmdBufferCreateInfo cmdCreateInfo;
        cmdCreateInfo.name = name.Value();
        cmdCreateInfo.pool = job.immediate ? this->immediateTransferPool : this->asyncTransferPool;
        cmdCreateInfo.usage = CoreGraphics::TransferQueueType;
        cmdCreateInfo.queryTypes = CoreGraphics::CmdBufferQueryBits::NoQueries;
        CoreGraphics::CmdBufferId uploadCommands = CoreGraphics::CreateCmdBuffer(cmdCreateInfo);

        CoreGraphics::CmdBufferBeginInfo beginInfo;
        beginInfo.submitOnce = true;
        beginInfo.submitDuringPass = false;
        beginInfo.resubmittable = false;
        CoreGraphics::CmdBeginRecord(uploadCommands, beginInfo);
        CoreGraphics::CmdBeginMarker(uploadCommands, NEBULA_MARKER_TRANSFER, name.Value());

        n_assert(job.name == job.streamData.stream->GetURI().LocalPath());

        // Perform mip loads
        uint mask = LoadMips(uploadCommands, streamData, bitsToLoad, texture);
        if (mask != 0x0)
        {
            pendingBits |= mask;

            // Then record mip finishes
            CoreGraphics::CmdBufferCreateInfo handoverCmdCreateInfo;
            handoverCmdCreateInfo.name = "Texture Mip Upload";
            handoverCmdCreateInfo.pool = job.immediate ? this->immediateHandoverPool : this->asyncHandoverPool;
            handoverCmdCreateInfo.usage = CoreGraphics::GraphicsQueueType;
            handoverCmdCreateInfo.queryTypes = CoreGraphics::CmdBufferQueryBits::NoQueries;

            CoreGraphics::CmdBufferId handoverCommands = CoreGraphics::CreateCmdBuffer(handoverCmdCreateInfo);
            CoreGraphics::CmdBeginRecord(handoverCommands, beginInfo);
            CoreGraphics::CmdBeginMarker(handoverCommands, NEBULA_MARKER_GRAPHICS, job.name.AsCharPtr());

            FinishMips(uploadCommands, handoverCommands, streamData, mask, texture, job.name.AsCharPtr());

            CoreGraphics::CmdEndMarker(handoverCommands);
            CoreGraphics::CmdEndRecord(handoverCommands);
            CoreGraphics::CmdBufferIdRelease(handoverCommands);

            CoreGraphics::CmdEndMarker(uploadCommands);
            CoreGraphics::CmdEndRecord(uploadCommands);
            CoreGraphics::CmdBufferIdRelease(uploadCommands);


            if (job.immediate)
            {
                CoreGraphics::SubmissionWaitEvent transferWait = CoreGraphics::SubmitCommandBuffers({ uploadCommands }, CoreGraphics::TransferQueueType, nullptr, "Texture mip upload");
                CoreGraphics::SubmissionWaitEvent graphicsWait = CoreGraphics::SubmitCommandBuffers({ handoverCommands }, CoreGraphics::GraphicsQueueType, { transferWait }, "Receive texture");
                CoreGraphics::DeferredDestroyCmdBuffer(uploadCommands);

                IndexT index = this->mipHandovers.FindIndex(job.id);
                if (index == InvalidIndex)
                    this->mipHandovers.Add(job.id, { MipHandoverLoaderThread{ .handoverSubmissionId = graphicsWait.timelineIndex, .bits = mask, .uploadBuffer = uploadCommands, .receiveBuffer = handoverCommands } });
                else
                    this->mipHandovers.ValueAtIndex(job.id, index).Append(MipHandoverLoaderThread{ .handoverSubmissionId = graphicsWait.timelineIndex, .bits = mask, .uploadBuffer = uploadCommands, .receiveBuffer = handoverCommands });
            }
            else
            {
                // If job is async, add to submit queue
                this->mipLoadsToSubmit.Enqueue(MipLoadMainThread{ .id = job.id, .bits = mask, .transferCmdBuf = uploadCommands, .graphicsCmdBuf = handoverCommands });
            }
        }
        else
        {
            CoreGraphics::CmdEndMarker(uploadCommands);
            CoreGraphics::CmdEndRecord(uploadCommands);
            CoreGraphics::CmdBufferIdRelease(uploadCommands);
            CoreGraphics::DestroyCmdBuffer(uploadCommands);
        }
    }
    if (job.loadState.pendingBits != 0x0)
    {
        // Check for pending handovers
        this->handoverLock.Enter();
        IndexT index = this->mipHandovers.FindIndex(job.id);
        if (index != InvalidIndex)
        {
            Util::Array<MipHandoverLoaderThread>& handovers = this->mipHandovers.ValueAtIndex(job.id, index);
            for (int i = 0; i < handovers.Size(); i++)
            {
                const MipHandoverLoaderThread& handover = handovers[i];
                if (CoreGraphics::PollSubmissionIndex(CoreGraphics::GraphicsQueueType, handover.handoverSubmissionId))
                {
                    // First, delete the initial buffer
                    CoreGraphics::DestroyCmdBuffer(handover.uploadBuffer);
                    CoreGraphics::DestroyCmdBuffer(handover.receiveBuffer);

                    loadedBits |= handover.bits;
                    pendingBits &= ~handover.bits;

                    TextureSetHighestLod(texture, streamData->numMips - 1 - Util::LastOne(loadedBits));

                    // Erase the handover entry for this job
                    handovers.EraseIndex(i);
                    i--;
                }
            }
            if (handovers.IsEmpty())
                this->mipHandovers.EraseIndex(job.id, index);
        }
        this->handoverLock.Leave();
    }
    else if (job.loadState.pendingBits == 0x0 && bitsToLoad == 0x0)
    {
        n_warning("Resource '%s' is stuck in an infinite state\n", job.name.AsCharPtr());
    }

    TextureIdRelease(texture);

    ret.loadedBits = loadedBits;
    ret.pendingBits = pendingBits;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureLoader::Unload(const Resources::ResourceId id)
{
    // Free streamer alloc
    this->streamDatas[id.loaderInstanceId].stream->MemoryUnmap();
    Memory::Free(Memory::ScratchHeap, this->streamDatas[id.loaderInstanceId].data);
    TextureId tex = id.resource;
    CoreGraphics::DestroyTexture(tex);
}

//------------------------------------------------------------------------------
/**
*/
void
TextureLoader::UpdateLoaderSyncState()
{
    Util::Array<MipLoadMainThread> mipLoads(128, 8);
    this->mipLoadsToSubmit.DequeueAll(mipLoads);
    for (const auto& mipLoad : mipLoads)
    {
        n_assert(mipLoad.transferCmdBuf != CoreGraphics::InvalidCmdBufferId);
        CoreGraphics::SubmissionWaitEvent transferEvent = CoreGraphics::SubmitCommandBuffers({ mipLoad.transferCmdBuf }, CoreGraphics::TransferQueueType, nullptr, "Texture mip upload");
        CoreGraphics::SubmissionWaitEvent graphicsEvent = CoreGraphics::SubmitCommandBuffers({ mipLoad.graphicsCmdBuf }, CoreGraphics::GraphicsQueueType, { transferEvent }, "Receive texture");

        this->handoverLock.Enter();
        IndexT index = this->mipHandovers.FindIndex(mipLoad.id);
        if (index == InvalidIndex)
            this->mipHandovers.Add(mipLoad.id, { MipHandoverLoaderThread{ .handoverSubmissionId = graphicsEvent.timelineIndex, .bits = mipLoad.bits, .uploadBuffer = mipLoad.transferCmdBuf, .receiveBuffer = mipLoad.graphicsCmdBuf } });
        else
            this->mipHandovers.ValueAtIndex(mipLoad.id, index).Append(MipHandoverLoaderThread{ .handoverSubmissionId = graphicsEvent.timelineIndex, .bits = mipLoad.bits, .uploadBuffer = mipLoad.transferCmdBuf, .receiveBuffer = mipLoad.graphicsCmdBuf });
        this->handoverLock.Leave();
    }
    mipLoads.Clear();
}

//------------------------------------------------------------------------------
/**
*/
uint
TextureLoader::LodMask(const _StreamData& stream, float lod, bool async) const
{
    TextureStreamData* texStreamData = static_cast<TextureStreamData*>(stream.data);
    uint numMipsRequested = stream.data ? Math::min(8u, (uint)texStreamData->numMips) : texStreamData->numMips;

    // Base case when Lod is 1.0f is to request 8 mips
    if (lod < 1.0f)
    {
        numMipsRequested = texStreamData->numMips - ((texStreamData->numMips - 1) * lod);
    }

    // Create mask containing all bits set to the lowest mip
    return (1 << numMipsRequested) - 1;
}

} // namespace CoreGraphics
