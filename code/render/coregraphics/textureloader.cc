//------------------------------------------------------------------------------
//  textureloader.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/textureloader.h"
#include "coregraphics/load/glimltypes.h"

namespace CoreGraphics
{

__ImplementClass(CoreGraphics::TextureLoader, 'TXLO', Resources::ResourceLoader);

using namespace CoreGraphics;
using namespace Resources;
using namespace IO;
//------------------------------------------------------------------------------
/**
*/
TextureLoader::TextureLoader()
{
    this->async = true;
    this->placeholderResourceName = "tex:system/white.dds";
    this->failResourceName = "tex:system/error.dds";

    this->streamerThreadName = "Texture Pool Streamer Thread";
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
TextureLoader::LoadFromStream(Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    N_SCOPE_ACCUM(CreateAndLoad, TextureStream);
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());

    // Map memory, we will keep the memory mapping so we can stream in LODs later
    void* srcData = stream->MemoryMap();
    uint srcDataSize = stream->GetSize();

    const int NumBasicLods = immediate ? 1000 : 5;

    // load using gliml
    gliml::context ctx;
    if (ctx.load_dds(srcData, srcDataSize))
    {
        // We have a total amount of mip maps, say 10
        int numMips = ctx.num_mipmaps(0);

        // That means the maximum mip is 9, [0..9]
        int maxMip = numMips - 1;

        // The lowest mip is halfway down the mip chain, so [5..9]
        int lowestMip = Math::max(0, maxMip - NumBasicLods);

        // This means we should load 10 - 5 = 5 mips
        int mipsToLoad = numMips - lowestMip;

        int depth = ctx.image_depth(0, 0);
        int width = ctx.image_width(0, 0);
        int height = ctx.image_height(0, 0);
        int layers = ctx.num_faces();

        auto streamData = (TextureStreamData*)Memory::Alloc(Memory::ScratchHeap, sizeof(TextureStreamData));
        streamData->mappedBuffer = srcData;
        streamData->mappedBufferSize = srcDataSize;
        streamData->ctx = ctx;
        streamData->lowestLod = lowestMip;
        streamData->maxMip = maxMip;
        this->streams[entry].stream = stream;
        this->streams[entry].data = streamData;

        CoreGraphics::PixelFormat::Code format = CoreGraphics::Gliml::ToPixelFormat(ctx);
        CoreGraphics::TextureType type = ctx.is_3d() ? CoreGraphics::Texture3D : (layers == 6 ? CoreGraphics::TextureCube : CoreGraphics::Texture2D);

        CoreGraphics::TextureCreateInfo textureInfo;
        textureInfo.width = width;
        textureInfo.height = height;
        textureInfo.depth = depth;
        textureInfo.mips = numMips;
        textureInfo.minMip = streamData->lowestLod;
        textureInfo.layers = layers;
        textureInfo.type = type;
        textureInfo.format = format;
        CoreGraphics::TextureId texture = CoreGraphics::CreateTexture(textureInfo);

        CoreGraphics::ImageSubresourceInfo subres(CoreGraphics::ImageAspect::ColorBits, streamData->lowestLod, mipsToLoad, 0, textureInfo.layers);

        // use resource submission
        CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockTransferSetupCommandBuffer();

        // Transition to transfer
        CoreGraphics::CmdBarrier(cmdBuf,
            CoreGraphics::PipelineStage::ImageInitial,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::BarrierDomain::Global,
            {
                TextureBarrierInfo
                {
                    texture,
                    subres
                }
            },
            nullptr);

        // now load texture by walking through all images and mips
        for (int i = 0; i < ctx.num_faces(); i++)
        {
            for (int j = streamData->lowestLod; j < ctx.num_mipmaps(i); j++)
            {
                // Perform a texture update
                CoreGraphics::TextureUpdate(
                    cmdBuf
                    , QueueType::TransferQueueType
                    , texture
                    , ctx.image_width(i, j)
                    , ctx.image_height(i, j)
                    , j
                    , subres.layer + i
                    , ctx.image_size(i, j)
                    , (byte*)ctx.image_data(i, j));
            }
        }

        // Transition back to read, and exchange queue ownership with the graphics queue
        CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, stream->GetURI().LocalPath().AsCharPtr());
        CoreGraphics::CmdBarrier(cmdBuf,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::BarrierDomain::Global,
            {
                TextureBarrierInfo
                {
                    texture,
                    subres
                }
            },
            nullptr,
            CoreGraphics::GetQueueIndex(QueueType::TransferQueueType),
            CoreGraphics::GetQueueIndex(QueueType::GraphicsQueueType));
        CoreGraphics::CmdEndMarker(cmdBuf);
        CoreGraphics::UnlockTransferSetupCommandBuffer();

        // Do the same barrier on the handover buffer, and exchange queue ownership with the graphics queue
        CoreGraphics::CmdBufferId handoverCmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer();

        CoreGraphics::CmdBeginMarker(handoverCmdBuf, NEBULA_MARKER_TRANSFER, stream->GetURI().LocalPath().AsCharPtr());

        // First duplicate the transfer queue barrier
        CoreGraphics::CmdBarrier(handoverCmdBuf,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::BarrierDomain::Global,
            {
                TextureBarrierInfo
                {
                    texture,
                    subres
                }
            },
            nullptr,
            CoreGraphics::GetQueueIndex(QueueType::TransferQueueType),
            CoreGraphics::GetQueueIndex(QueueType::GraphicsQueueType));

        // Then perform the actual image layout change
        CoreGraphics::CmdBarrier(handoverCmdBuf,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::PipelineStage::AllShadersRead,
            CoreGraphics::BarrierDomain::Global,
            {
                TextureBarrierInfo
                {
                    texture,
                    subres
                }
            });
        CoreGraphics::CmdEndMarker(handoverCmdBuf);
        CoreGraphics::UnlockGraphicsSetupCommandBuffer();

#if NEBULA_GRAPHICS_DEBUG
        ObjectSetName(texture, stream->GetURI().LocalPath().AsCharPtr());
#endif
        return texture;
    }
    stream->MemoryUnmap();
    return Resources::InvalidResourceUnknownId;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureLoader::Unload(const Resources::ResourceId id)
{
    // Free streamer alloc
    this->streams[id.cacheInstanceId].stream->MemoryUnmap();
    Memory::Free(Memory::ScratchHeap, this->streams[id.cacheInstanceId].data);
    TextureId tex;
    tex.resourceId = id.resourceId;
    tex.resourceType = id.resourceType;
    CoreGraphics::DestroyTexture(tex);
}

//------------------------------------------------------------------------------
/**
*/
void
TextureLoader::StreamMaxLOD(const Resources::ResourceId& id, const float lod, bool immediate)
{
    N_SCOPE_ACCUM(StreamMaxLOD, TextureStream);
    TextureStreamData* streamData = (TextureStreamData*)this->streams[id.cacheInstanceId].data;

    // if the lod is undefined, just add 1 mip
    IndexT adjustedLod = Math::max(0, (IndexT)Math::ceil(streamData->maxMip * lod));

    // abort if the lod is already higher
    if (streamData->lowestLod <= (uint32_t)adjustedLod)
        return;

    TextureId texture;
    texture.resourceId = id.resourceId;
    texture.resourceType = id.resourceType;

    const gliml::context& ctx = streamData->ctx;
    CoreGraphics::ImageSubresourceInfo subres(CoreGraphics::ImageAspect::ColorBits, adjustedLod, streamData->lowestLod, 0, ctx.num_faces());

    // use resource submission
    CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockTransferSetupCommandBuffer();

    // transition to transfer
    CoreGraphics::CmdBarrier(cmdBuf,
        CoreGraphics::PipelineStage::ImageInitial,
        CoreGraphics::PipelineStage::TransferWrite,
        CoreGraphics::BarrierDomain::Global,
        {
            TextureBarrierInfo
            {
                texture,
                subres
            }
        });

    // now load texture by walking through all images and mips
    for (int i = 0; i < subres.layerCount; i++)
    {
        for (int j = adjustedLod; j < streamData->lowestLod; j++)
        {
            // Perform a texture update
            CoreGraphics::TextureUpdate(
                cmdBuf
                , QueueType::TransferQueueType
                , texture
                , ctx.image_width(i, j)
                , ctx.image_height(i, j)
                , j
                , subres.layer + i
                , ctx.image_size(i, j)
                , (byte*)ctx.image_data(i, j));
        }
    }

    // Transition image to read
    CoreGraphics::CmdBarrier(cmdBuf,
        CoreGraphics::PipelineStage::TransferWrite,
        CoreGraphics::PipelineStage::TransferWrite,
        CoreGraphics::BarrierDomain::Global,
        {
            TextureBarrierInfo
            {
                texture,
                subres
            }
        });
    CoreGraphics::UnlockTransferSetupCommandBuffer();

    // perform final transition on graphics queue
    CoreGraphics::CmdBufferId handoverCmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer();

    CoreGraphics::CmdBarrier(handoverCmdBuf,
        CoreGraphics::PipelineStage::TransferWrite,
        CoreGraphics::PipelineStage::TransferWrite,
        CoreGraphics::BarrierDomain::Global,
        {
            TextureBarrierInfo
            {
                texture,
                subres
            }
        });

    CoreGraphics::CmdBarrier(handoverCmdBuf,
        CoreGraphics::PipelineStage::TransferWrite,
        CoreGraphics::PipelineStage::AllShadersRead,
        CoreGraphics::BarrierDomain::Global,
        {
            TextureBarrierInfo
            {
                texture,
                subres
            }
        });

    CoreGraphics::UnlockGraphicsSetupCommandBuffer();
    streamData->lowestLod = adjustedLod;

    // When all that is done, set the highest lod which triggers a new image view creation
    TextureSetHighestLod(texture, adjustedLod);
}

} // namespace CoreGraphics
