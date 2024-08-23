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
    Util::Array<Resources::PartialLoadBits> partialLoadBits;
    Util::Array<CoreGraphics::CmdBufferId> transferCommands, handoverCommands;
};

using namespace CoreGraphics;
using namespace Resources;
using namespace IO;
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
    this->transferPool = CoreGraphics::CreateCmdBufferPool(cmdPoolInfo);
    cmdPoolInfo.queue = CoreGraphics::QueueType::GraphicsQueueType;
    this->handoverPool = CoreGraphics::CreateCmdBufferPool(cmdPoolInfo);
}

//------------------------------------------------------------------------------
/**
*/
TextureLoader::~TextureLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceUnknownId
TextureLoader::InitializeResource(Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    N_SCOPE_ACCUM(CreateAndLoad, TextureStream);
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());

    // Map memory, we will keep the memory mapping so we can stream in LODs later
    void* srcData = stream->MemoryMap();
    uint srcDataSize = stream->GetSize();
    ResourceName name = this->names[entry];

    Resources::ResourceUnknownId ret = InvalidResourceUnknownId;

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
        streamData->partialLoadBits = Util::Array<PartialLoadBits>();
        streamData->handoverCommands = Util::Array<CoreGraphics::CmdBufferId>();
        streamData->transferCommands = Util::Array<CoreGraphics::CmdBufferId>();
        memset(streamData->layers, 0x0, sizeof(streamData->layers));
        for (uint i = 0; i < numMips; i++)
        {
            streamData->layers[i] = (1 << layers) - 1;
        }

        this->streams[entry].stream = stream;
        this->streams[entry].data = streamData;

        CoreGraphics::PixelFormat::Code format = CoreGraphics::Gliml::ToPixelFormat(ctx);
        CoreGraphics::TextureType type = ctx.is_3d() ? CoreGraphics::Texture3D : (layers == 6 ? CoreGraphics::TextureCube : CoreGraphics::Texture2D);

        CoreGraphics::TextureCreateInfo textureInfo;
        textureInfo.name = name.Value();
        textureInfo.width = width;
        textureInfo.height = height;
        textureInfo.depth = depth;
        textureInfo.mips = numMips;
        textureInfo.minMip = numMips - 1;
        textureInfo.layers = layers;
        textureInfo.type = type;
        textureInfo.format = format;
        if (immediate)
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
        return texture;
    }

    stream->MemoryUnmap();
    return ret;
}

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
uint
TextureLoader::StreamResource(const Resources::ResourceId entry, IndexT frameIndex, uint requestedBits)
{
    // Get resource data
    ResourceLoader::StreamData& stream = this->streams[entry.loaderInstanceId];
    TextureStreamData* streamData = static_cast<TextureStreamData*>(stream.data);
    ResourceName name = this->names[entry.loaderInstanceId];

    uint loadedBits = this->loadedBits[entry.loaderInstanceId];
    uint ret = loadedBits;
    uint bitsToLoad = requestedBits;

    // Setup texture id
    TextureId texture = entry.resource;
    TextureIdAcquire(texture);

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

    cmdCreateInfo.name = "Texture Mip Upload";
    cmdCreateInfo.pool = this->handoverPool;
    cmdCreateInfo.usage = CoreGraphics::GraphicsQueueType;
    cmdCreateInfo.queryTypes = CoreGraphics::CmdBufferQueryBits::NoQueries;
    CoreGraphics::CmdBufferId handoverCommands = CoreGraphics::CreateCmdBuffer(cmdCreateInfo);

    CoreGraphics::CmdBeginRecord(transferCommands, beginInfo);
    CoreGraphics::CmdBeginRecord(handoverCommands, beginInfo);

    CoreGraphics::CmdBeginMarker(transferCommands, NEBULA_MARKER_TRANSFER, name.Value());
    CoreGraphics::CmdBeginMarker(handoverCommands, NEBULA_MARKER_GRAPHICS, name.Value());

    // First, poll all submissions to find what's finished loading
    for (IndexT i = 0; i < streamData->partialLoadBits.Size(); i++)
    {
        auto& bits = streamData->partialLoadBits[i];

        // Remove pending bits from requests
        bitsToLoad &= ~bits.bits;

        n_assert(bits.submissionId != UINT64_MAX);
        if (CoreGraphics::PollSubmissionIndex(CoreGraphics::TransferQueueType, bits.submissionId))
        {
            // Handover mips to the graphics queue
            FinishMips(transferCommands, handoverCommands, streamData, bits.bits, texture, name.Value());
            this->finishedResources.Append(entry);

            ret |= bits.bits;
            streamData->partialLoadBits.EraseIndex(i);
            i--;
        }
    }

    CoreGraphics::CmdEndMarker(handoverCommands);
    CoreGraphics::CmdEndMarker(transferCommands);

    CoreGraphics::CmdEndRecord(handoverCommands);
    CoreGraphics::CmdEndRecord(transferCommands);

    CoreGraphics::CmdBufferIdRelease(handoverCommands);
    CoreGraphics::CmdBufferIdRelease(transferCommands);
    streamData->handoverCommands.Append(handoverCommands);
    streamData->transferCommands.Append(transferCommands);

    bitsToLoad &= ~ret;
    if (bitsToLoad != 0x0)
    {
        CoreGraphics::CmdBufferCreateInfo cmdCreateInfo;
        cmdCreateInfo.name = name.Value();
        cmdCreateInfo.pool = this->transferPool;
        cmdCreateInfo.usage = CoreGraphics::TransferQueueType;
        cmdCreateInfo.queryTypes = CoreGraphics::CmdBufferQueryBits::NoQueries;
        CoreGraphics::CmdBufferId uploadCommands = CoreGraphics::CreateCmdBuffer(cmdCreateInfo);

        CoreGraphics::CmdBufferBeginInfo beginInfo;
        beginInfo.submitOnce = true;
        beginInfo.submitDuringPass = false;
        beginInfo.resubmittable = false;
        CoreGraphics::CmdBeginRecord(uploadCommands, beginInfo);
        CoreGraphics::CmdBeginMarker(uploadCommands, NEBULA_MARKER_TRANSFER, name.Value());

        // Prepare return state
        uint mask = LoadMips(uploadCommands, streamData, bitsToLoad, texture);

        // Get the submission index associated with the load this frame
        streamData->partialLoadBits.Append(PartialLoadBits {.bits = mask, .submissionId = UINT64_MAX, .cmdBuf = uploadCommands});

        CoreGraphics::CmdEndMarker(uploadCommands);
        CoreGraphics::CmdEndRecord(uploadCommands);
        CoreGraphics::CmdBufferIdRelease(uploadCommands);

        this->asyncSection.Enter();
        this->partiallyCompleteResources.Append(entry);
        this->asyncSection.Leave();
    }

    if (loadedBits != ret)
    {
        TextureSetHighestLod(texture, streamData->numMips - 1 - Util::LastOne(ret));
    }
    TextureIdRelease(texture);

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureLoader::Unload(const Resources::ResourceId id)
{
    // Free streamer alloc
    this->streams[id.loaderInstanceId].stream->MemoryUnmap();
    Memory::Free(Memory::ScratchHeap, this->streams[id.loaderInstanceId].data);
    TextureId tex = id.resource;
    CoreGraphics::DestroyTexture(tex);
}

//------------------------------------------------------------------------------
/**
*/
void
TextureLoader::UpdateLoaderSyncState()
{
    for (auto& entry : this->partiallyCompleteResources)
    {
        ResourceLoader::StreamData& stream = this->streams[entry.loaderInstanceId];
        TextureStreamData* streamData = static_cast<TextureStreamData*>(stream.data);
        for (auto& bits : streamData->partialLoadBits)
        {
            bits.submissionId = CoreGraphics::NextSubmissionIndex(CoreGraphics::TransferQueueType);
            CoreGraphics::SubmitCommandBuffers({ bits.cmdBuf }, CoreGraphics::TransferQueueType, "Texture Loader Stream");
            CoreGraphics::DestroyCmdBuffer(bits.cmdBuf);
        }
    }
    this->partiallyCompleteResources.Clear();

    for (auto& entry : this->finishedResources)
    {
        ResourceLoader::StreamData& stream = this->streams[entry.loaderInstanceId];
        TextureStreamData* streamData = static_cast<TextureStreamData*>(stream.data);
        CoreGraphics::SubmitCommandBuffers(streamData->transferCommands, CoreGraphics::TransferQueueType, "Finished Texture Send");
        CoreGraphics::SubmitCommandBuffers(streamData->handoverCommands, CoreGraphics::GraphicsQueueType, "Finished Textures Receive");

        for (auto cmd : streamData->transferCommands)
            CoreGraphics::DestroyCmdBuffer(cmd);
        for (auto cmd : streamData->handoverCommands)
            CoreGraphics::DestroyCmdBuffer(cmd);
        streamData->transferCommands.Clear();
        streamData->handoverCommands.Clear();
    }
    this->finishedResources.Clear();
}

//------------------------------------------------------------------------------
/**
*/
uint
TextureLoader::LodMask(const Ids::Id32 entry, float lod, bool stream) const
{
    ResourceLoader::StreamData& streamData = this->streams[entry];
    TextureStreamData* texStreamData = static_cast<TextureStreamData*>(streamData.data);
    uint numMipsRequested = stream ? Math::min(8u, (uint)texStreamData->numMips) : texStreamData->numMips;

    // Base case when Lod is 1.0f is to request 8 mips
    if (lod < 1.0f)
    {
        numMipsRequested = texStreamData->numMips - ((texStreamData->numMips - 1) * lod);
    }

    // Create mask containing all bits set to the lowest mip
    return (1 << numMipsRequested) - 1;
}

} // namespace CoreGraphics
