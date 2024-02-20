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
        streamData->mappedBuffer = srcData;
        streamData->mappedBufferSize = srcDataSize;
        streamData->ctx = ctx;

        streamData->numMips = numMips;

        streamData->numLayers = layers;
        streamData->nextLayerToLoad = 0;
        streamData->numLayersToLoad = layers;

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
UploadToTexture(const CoreGraphics::TextureId texture, const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::CmdBufferId handoverCmdBuf, gliml::context& ctx, uint layer, uint mip)
{
    // Attempt to upload
    CoreGraphics::TextureSubresourceInfo subres(CoreGraphics::ImageBits::ColorBits, mip, 1, layer, 1);

    SizeT alignment = CoreGraphics::PixelFormat::ToTexelSize(TextureGetPixelFormat(texture));
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

        CoreGraphics::CmdHandover(
            cmdBuf, handoverCmdBuf
            , CoreGraphics::PipelineStage::TransferWrite
            , CoreGraphics::PipelineStage::AllShadersRead
            , {
                TextureBarrierInfo
                {
                    texture,
                    subres
                }
            }
            , nullptr
            , CoreGraphics::GetQueueIndex(QueueType::TransferQueueType)
            , CoreGraphics::GetQueueIndex(QueueType::GraphicsQueueType)
        );
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
uint
LoadMips(TextureStreamData* streamData, uint bitsToLoad, const CoreGraphics::TextureId texture, const char* name)
{
    // use resource submission
    CoreGraphics::TransferLock lock = CoreGraphics::LockTransfer();
    CoreGraphics::CmdBeginMarker(lock.transferBuffer, NEBULA_MARKER_TRANSFER, name);
    CoreGraphics::CmdBeginMarker(lock.setupBuffer, NEBULA_MARKER_TRANSFER, name);

    uint mipIndexToLoad = Util::FirstOne(bitsToLoad);
    uint loadedBits = 0x0;

    while (bitsToLoad != 0x0)
    {
        streamData->nextLayerToLoad = 0;
        uint mipToLoad = streamData->numMips - 1 - mipIndexToLoad;
        while (streamData->nextLayerToLoad < streamData->numLayersToLoad)
        {
            // Attempt to upload, if it fails we continue from here next time
            if (!UploadToTexture(texture, lock.transferBuffer, lock.setupBuffer, streamData->ctx, streamData->nextLayerToLoad, mipToLoad))
            {
                // If upload fails, escape the loop
                goto quit_loop;
            }

            streamData->nextLayerToLoad++;
        }

        loadedBits |= 1 << mipIndexToLoad;
        bitsToLoad &= ~(1 << mipIndexToLoad);
        mipIndexToLoad = Util::FirstOne(bitsToLoad);
    }

quit_loop:

    CoreGraphics::CmdEndMarker(lock.transferBuffer);
    CoreGraphics::CmdEndMarker(lock.setupBuffer);
    return loadedBits;
}

//------------------------------------------------------------------------------
/**
*/
uint
TextureLoader::StreamResource(const Resources::ResourceId entry, uint requestedBits)
{
    // Get resource data
    ResourceLoader::StreamData& stream = this->streams[entry.loaderInstanceId];
    uint loadedBits = this->loadedBits[entry.loaderInstanceId];
    uint bitsToLoad = requestedBits ^ loadedBits;
    uint ret = loadedBits;
    if (bitsToLoad != 0x0)
    {
        TextureStreamData* streamData = static_cast<TextureStreamData*>(stream.data);
        ResourceName name = this->names[entry.loaderInstanceId];

        // Setup texture id
        TextureId texture;
        texture.resourceId = entry.resourceId;
        texture.resourceType = entry.resourceType;
        TextureIdAcquire(texture);

        // Prepare return state
        uint mask = LoadMips(streamData, bitsToLoad, texture, name.Value());
        ret |= mask;

        TextureSetHighestLod(texture, streamData->numMips - 1 - Util::LastOne(ret));

        TextureIdRelease(texture);
    }

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

    // Base case when Lod is 1.0f is to request 8 mips
    if (lod < 1.0f)
    {
        numMipsRequested = streamData->numMips - ((streamData->numMips - 1) * lod);
    }

    // Create mask containing all bits set to the lowest mip
    uint mask = (0x1 << numMipsRequested) - 1;
    return mask;
}

} // namespace CoreGraphics
