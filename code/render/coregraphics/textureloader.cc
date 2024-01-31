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
    char nextLayerToLoad;
    ubyte numLayersToLoad, numLayers;
    Util::Array<Resources::PartialLoadBits> partialLoadBits;
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
UploadToTexture(const CoreGraphics::TextureId texture, const CoreGraphics::CmdBufferId cmdBuf, gliml::context& ctx, uint layer, uint mip)
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
        CoreGraphics::CmdBarrier(cmdBuf
            , CoreGraphics::PipelineStage::ImageInitial
            , CoreGraphics::PipelineStage::TransferWrite
            , CoreGraphics::BarrierDomain::Global
            , {
                TextureBarrierInfo
                {
                    texture,
                    subres
                }
            });

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
MakeLodBits(uint mip, uint layer)
{
    uint mipBits = (1 << mip) - 1;
    uint layerBits = (1 << layer) - 1;
    return (mipBits & 0xFFFF) | ((layerBits << 16) & 0xFFFF);
}

//------------------------------------------------------------------------------
/**
*/
uint
MakeLayerBits(uint layer)
{
    return (1 << layer) - 1;
}

//------------------------------------------------------------------------------
/**
*/
uint
GetLayerBits(uint bits)
{
    return (bits >> 16) & 0xFFFF;
}

//------------------------------------------------------------------------------
/**
*/
uint
GetMipBits(uint bits)
{
    return bits & 0xFFFF;
}

//------------------------------------------------------------------------------
/**
*/
uint
LoadMips(TextureStreamData* streamData, uint bitsToLoad, const CoreGraphics::TextureId texture, const char* name)
{
    // use resource submission
    CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockTransferSetupCommandBuffer();
    CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, name);

    uint mipBits = GetMipBits(bitsToLoad), layerBits = GetLayerBits(bitsToLoad);
    uint loadedBits = 0x0;

    while (mipBits != 0x0)
    {
        uint mipIndexToLoad = Util::FirstOne(mipBits);
        uint mipToLoad = streamData->numMips - 1 - mipIndexToLoad;
        while (layerBits != 0x0)
        {
            uint layerIndexToLoad = Util::FirstOne(layerBits);
            uint layerToLoad = streamData->numLayers - 1 - layerIndexToLoad;

            // Attempt to upload, if it fails we continue from here next time
            if (!UploadToTexture(texture, cmdBuf, streamData->ctx, layerToLoad, mipToLoad))
            {
                // If upload fails, escape the loop
                goto quit_loop;
            }

            layerBits &= ~(1 << layerIndexToLoad);
        }

        mipBits &= ~(1 << mipIndexToLoad);
        layerBits = MakeLayerBits(streamData->numLayers);
    }
    loadedBits = mipBits | (layerBits << 16);

quit_loop:

    CoreGraphics::CmdEndMarker(cmdBuf);
    CoreGraphics::UnlockTransferSetupCommandBuffer();

    return loadedBits;
}

//------------------------------------------------------------------------------
/**
*/
void FinishMips(TextureStreamData* streamData, uint bitsToLoad, const CoreGraphics::TextureId texture, const char* name)
{
    CoreGraphics::CmdBufferId handoverCmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer();
    CoreGraphics::CmdBeginMarker(handoverCmdBuf, NEBULA_MARKER_TRANSFER, name);

    // Finish the mips by handing them over 
    Util::FixedArray<TextureBarrierInfo> barriers(Util::CountBits(bitsToLoad));
    uint mipBits = GetMipBits(bitsToLoad), layerBits = GetLayerBits(bitsToLoad);
    uint mipIndexToLoad = Util::FirstOne(mipBits);
    uint layerIndexToLoad = Util::FirstOne(layerBits);
    uint barrierCounter = 0;
    while (mipBits != 0x0)
    {
        uint mipIndexToLoad = Util::FirstOne(mipBits);
        uint mipToLoad = streamData->numMips - 1 - mipIndexToLoad;

        while (layerBits != 0x0)
        {
            uint layerIndexToLoad = Util::FirstOne(layerBits);
            uint layerToLoad = streamData->numLayers - 1 - layerIndexToLoad;
            CoreGraphics::TextureSubresourceInfo subres(CoreGraphics::ImageBits::ColorBits, mipToLoad, 1, layerToLoad, 1);
            barriers[barrierCounter++] = TextureBarrierInfo{ .tex = texture, .subres = subres };
            layerBits &= ~(1 << layerIndexToLoad);
        }

        mipBits &= ~(1 << mipIndexToLoad);
        layerBits = MakeLayerBits(streamData->numLayers);
    }

    CoreGraphics::CmdBarrier(
        handoverCmdBuf
        , CoreGraphics::PipelineStage::TransferWrite
        , CoreGraphics::PipelineStage::AllShadersRead
        , CoreGraphics::BarrierDomain::Global
        , barriers
        , CoreGraphics::GetQueueIndex(QueueType::TransferQueueType)
        , CoreGraphics::GetQueueIndex(QueueType::GraphicsQueueType)
    );

    CoreGraphics::CmdEndMarker(handoverCmdBuf);
    CoreGraphics::UnlockGraphicsSetupCommandBuffer();
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

    // Setup texture id
    TextureId texture;
    texture.resourceId = entry.resourceId;
    texture.resourceType = entry.resourceType;
    TextureIdAcquire(texture);

    // First, poll all submissions to find what's finished loading
    for (IndexT i = 0; i < streamData->partialLoadBits.Size(); i++)
    {
        auto& bits = streamData->partialLoadBits[i];
        if (CoreGraphics::PollSubmissionIndex(CoreGraphics::TransferQueueType, bits.submissionId))
        {
            ret |= bits.bits;
            streamData->partialLoadBits.EraseIndex(i);
            i--;

            // Handover mips to the graphics queue
            FinishMips(streamData, ret, texture, name.Value());
        }
    }

    uint bitsToLoad = requestedBits ^ ret;
    if (bitsToLoad != 0x0)
    {

        // Prepare return state
        uint mask = LoadMips(streamData, bitsToLoad, texture, name.Value());
        //ret |= mask;

        // Get the submission index associated with the load this frame
        uint64 submissionId = CoreGraphics::NextSubmissionIndex(CoreGraphics::TransferQueueType);
        streamData->partialLoadBits.Append(PartialLoadBits{.bits = mask, .submissionId = submissionId});
    }

    if (loadedBits != ret)
        TextureSetHighestLod(texture, streamData->numMips - 1 - Util::LastOne(ret));
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
    TextureId tex;
    tex.resourceId = id.resourceId;
    tex.resourceType = id.resourceType;
    CoreGraphics::DestroyTexture(tex);
}

//------------------------------------------------------------------------------
/**
*/
uint
TextureLoader::LodMask(const Ids::Id32 entry, float lod) const
{
    ResourceLoader::StreamData& stream = this->streams[entry];
    TextureStreamData* streamData = static_cast<TextureStreamData*>(stream.data);
    uint numMipsRequested = Math::min(8u, (uint)streamData->numMips);
    uint numLayers = streamData->numLayersToLoad;
    uint layerBits = ((1 << streamData->numLayersToLoad) - 1) << 16;

    // Base case when Lod is 1.0f is to request 8 mips
    if (lod < 1.0f)
    {
        numMipsRequested = streamData->numMips - ((streamData->numMips - 1) * lod);
    }

    // Create mask containing all bits set to the lowest mip
    uint mask = (1 << numMipsRequested) - 1;
    return mask | layerBits;
}

} // namespace CoreGraphics
