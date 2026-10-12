//------------------------------------------------------------------------------
//  bloomcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "frame/framesubgraph.h"
#include "frame/framecode.h"
#include "coregraphics/resourcetable.h"
#include "coregraphics/barrier.h"
#include "coregraphics/texture.h"
#include "graphics/graphicsserver.h"
#include "bloomcontext.h"

#include "gpulang/render/system_shaders/bloom.h"

#include "frame/default.h"
namespace PostEffects
{

__ImplementPluginContext(PostEffects::BloomContext);
struct
{
    CoreGraphics::ShaderProgramId intermediateProgram;
    CoreGraphics::ShaderProgramId mergeProgram;
    CoreGraphics::ShaderId shader;

    CoreGraphics::ResourceTableId resourceTable;
    CoreGraphics::BufferId constants;

    CoreGraphics::TextureId intermediateBloomTexture;
    CoreGraphics::TextureId dirtMask;
    Util::FixedArray<CoreGraphics::TextureViewId> intermediateBloomBufferViews;
    uint numMips;
    Bloom::BloomUniforms::STRUCT uniforms;

} bloomState;

//------------------------------------------------------------------------------
/**
    Dirt.x is 0 for the white placeholder.
    Dirt.yz is the reciprocal of DirtMask mip 0.
*/
static void
WriteDirtUniform(const CoreGraphics::TextureId texture)
{
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(texture);
    n_assert(dims.width > 0);
    n_assert(dims.height > 0);
    bloomState.uniforms.Dirt[0] = texture == CoreGraphics::White2D ? 0.0f : 1.0f;
    bloomState.uniforms.Dirt[1] = 1.0f / float(dims.width);
    bloomState.uniforms.Dirt[2] = 1.0f / float(dims.height);
    bloomState.uniforms.Dirt[3] = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
BloomContext::BloomContext()
{
}

//------------------------------------------------------------------------------
/**
*/
BloomContext::~BloomContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void
BloomContext::Setup()
{
    using namespace CoreGraphics;

    bloomState.shader = ShaderGet("shd:system_shaders/bloom.gplb");
    bloomState.intermediateProgram = ShaderGetProgram(bloomState.shader, ShaderFeatureMask("Intermediate"));
    bloomState.mergeProgram = ShaderGetProgram(bloomState.shader, ShaderFeatureMask("Merge"));
    bloomState.resourceTable = ShaderCreateResourceTable(bloomState.shader, NEBULA_BATCH_GROUP);

    TextureDimensions dims = TextureGetDimensions(FrameScript_default::Texture_BloomBuffer());

    BufferCreateInfo bufInfo;
    bufInfo.byteSize = sizeof(Bloom::BloomUniforms::STRUCT);
    bufInfo.usageFlags = BufferUsage::ConstantBuffer | BufferUsage::TransferDestination;
    bufInfo.mode = DeviceAndHost;
    bufInfo.queueSupport = ComputeQueueSupport;
    bloomState.constants = CreateBuffer(bufInfo);

    uint mips = TextureGetNumMips(FrameScript_default::Texture_LightBuffer());
    bloomState.numMips = mips;

    CoreGraphics::TextureCreateInfo intermediateTextureInfo;
    intermediateTextureInfo.format = TextureGetPixelFormat(FrameScript_default::Texture_BloomBuffer());
    intermediateTextureInfo.width = dims.width;
    intermediateTextureInfo.height = dims.height;
    intermediateTextureInfo.usage = CoreGraphics::TextureUsage::ReadWrite;
    intermediateTextureInfo.mips = CoreGraphics::TextureAutoMips;
    bloomState.intermediateBloomTexture = CoreGraphics::CreateTexture(intermediateTextureInfo);
    bloomState.intermediateBloomBufferViews.Resize(mips);
    for (IndexT i = 0; i < mips; i++)
    {
        TextureViewCreateInfo inf;
        inf.format = intermediateTextureInfo.format;
        inf.startMip = i;
        inf.numMips = 1;
        inf.tex = bloomState.intermediateBloomTexture;
        bloomState.intermediateBloomBufferViews[i] = CreateTextureView(inf);
    }

    bloomState.uniforms.Mips = mips;
    for (IndexT i = 0; i < mips; i++)
    {
        float mipWidth = float(Math::max(1, int(dims.width >> i)));
        float mipHeight = float(Math::max(1, int(dims.height >> i)));
        bloomState.uniforms.Resolutions[i][0] = mipWidth;
        bloomState.uniforms.Resolutions[i][1] = mipHeight;
        bloomState.uniforms.Resolutions[i][2] = 1.0f / mipWidth;
        bloomState.uniforms.Resolutions[i][3] = 1.0f / mipHeight;
    }
    if (bloomState.dirtMask == InvalidTextureId)
        bloomState.dirtMask = White2D;
    WriteDirtUniform(bloomState.dirtMask);
    BufferUpdate(bloomState.constants, bloomState.uniforms);

    ResourceTableSetTexture(bloomState.resourceTable, { FrameScript_default::Texture_LightBuffer(), Bloom::Input::BINDING });
    ResourceTableSetTexture(bloomState.resourceTable, { bloomState.dirtMask, Bloom::DirtMask::BINDING });
    ResourceTableSetRWTexture(bloomState.resourceTable, { FrameScript_default::Texture_BloomBuffer(), Bloom::BloomOutput::BINDING });
    ResourceTableSetTexture(bloomState.resourceTable, { bloomState.intermediateBloomTexture, Bloom::Intermediate::BINDING });
    for (IndexT i = 0; i < mips; i++)
    {
        ResourceTableSetRWTexture(bloomState.resourceTable, { bloomState.intermediateBloomBufferViews[i], Bloom::BloomIntermediate::BINDING, i });
    }

    ResourceTableSetConstantBuffer(bloomState.resourceTable, { bloomState.constants, Bloom::BloomUniforms::BINDING });
    ResourceTableCommitChanges(bloomState.resourceTable);

    FrameScript_default::Bind_BloomIntermediate(Frame::TextureImport(bloomState.intermediateBloomTexture));

    FrameScript_default::RegisterSubgraph_BloomIntermediate_Compute([](const CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, bloomState.intermediateProgram, queue);
        CmdSetResourceTable(cmdBuf, bloomState.resourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

        TextureDimensions texDims = TextureGetDimensions(FrameScript_default::Texture_LightBuffer());
        bloomState.uniforms.Mips = bloomState.numMips;
        for (IndexT i = 0; i < bloomState.numMips; i++)
        {
            bloomState.uniforms.Resolutions[i][0] = float(Math::max(1, viewport.width() >> i));
            bloomState.uniforms.Resolutions[i][1] = float(Math::max(1, viewport.height() >> i));
            bloomState.uniforms.Resolutions[i][2] = 1.0f / float(Math::max(1, int(texDims.width >> i)));
            bloomState.uniforms.Resolutions[i][3] = 1.0f / float(Math::max(1, int(texDims.height >> i)));
        }
        CmdUpdateBuffer(cmdBuf, bloomState.constants, 0, sizeof(bloomState.uniforms), &bloomState.uniforms);

        for (int mip = int(bloomState.numMips) - 1; mip >= 0; mip--)
        {
            if (mip < int(bloomState.numMips) - 1)
            {
                CmdBarrier(cmdBuf, PipelineStage::ComputeShaderWrite, PipelineStage::ComputeShaderRead, BarrierDomain::Global);
            }

            uint mipWidth = Math::max(1, viewport.width() >> mip);
            uint mipHeight = Math::max(1, viewport.height() >> mip);
            uint dispatchX = Math::divandroundup(mipWidth, 14);
            uint dispatchY = Math::divandroundup(mipHeight, 14);
            CmdDispatch(cmdBuf, dispatchX, dispatchY, mip + 1);
        }
    }, nullptr, {
        { FrameScript_default::TextureIndex::LightBuffer, PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::BloomIntermediate, PipelineStage::ComputeShaderWrite }
    });

    FrameScript_default::RegisterSubgraph_BloomMerge_Compute([](const CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, bloomState.mergeProgram, queue);
        CmdSetResourceTable(cmdBuf, bloomState.resourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = Math::divandroundup(viewport.width(), 256);
        uint dispatchY = viewport.height();
        CmdDispatch(cmdBuf, dispatchX, dispatchY, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::BloomIntermediate, PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::BloomBuffer, PipelineStage::ComputeShaderWrite }
    });
}

//------------------------------------------------------------------------------
/**
*/
void
BloomContext::Resize(const uint framescriptHash, SizeT width, SizeT height)
{
    if (framescriptHash == FrameScript_default::ID)
    {
        using namespace CoreGraphics;
        TextureDimensions dims = TextureGetDimensions(FrameScript_default::Texture_BloomBuffer());

        for (auto& view : bloomState.intermediateBloomBufferViews)
        {
            DestroyTextureView(view);
        }
        bloomState.intermediateBloomBufferViews.Clear();
        DestroyTexture(bloomState.intermediateBloomTexture);

        uint mips = TextureGetNumMips(FrameScript_default::Texture_LightBuffer());
        bloomState.numMips = mips;

        CoreGraphics::TextureCreateInfo intermediateTextureInfo;
        intermediateTextureInfo.format = TextureGetPixelFormat(FrameScript_default::Texture_BloomBuffer());
        intermediateTextureInfo.width = dims.width;
        intermediateTextureInfo.height = dims.height;
        intermediateTextureInfo.usage = CoreGraphics::TextureUsage::ReadWrite;
        intermediateTextureInfo.mips = CoreGraphics::TextureAutoMips;
        bloomState.intermediateBloomTexture = CoreGraphics::CreateTexture(intermediateTextureInfo);
        bloomState.intermediateBloomBufferViews.Resize(mips);
        for (IndexT i = 0; i < mips; i++)
        {
            TextureViewCreateInfo inf;
            inf.format = intermediateTextureInfo.format;
            inf.startMip = i;
            inf.numMips = 1;
            inf.tex = bloomState.intermediateBloomTexture;
            bloomState.intermediateBloomBufferViews[i] = CreateTextureView(inf);
        }

        bloomState.uniforms.Mips = mips;
        for (IndexT i = 0; i < mips; i++)
        {
            float mipWidth = float(Math::max(1, int(dims.width >> i)));
            float mipHeight = float(Math::max(1, int(dims.height >> i)));
            bloomState.uniforms.Resolutions[i][0] = mipWidth;
            bloomState.uniforms.Resolutions[i][1] = mipHeight;
            bloomState.uniforms.Resolutions[i][2] = 1.0f / mipWidth;
            bloomState.uniforms.Resolutions[i][3] = 1.0f / mipHeight;
        }
        BufferUpdate(bloomState.constants, bloomState.uniforms);

        ResourceTableSetTexture(bloomState.resourceTable, {FrameScript_default::Texture_LightBuffer(), Bloom::Input::BINDING});
        ResourceTableSetRWTexture(
            bloomState.resourceTable, {FrameScript_default::Texture_BloomBuffer(), Bloom::BloomOutput::BINDING}
        );
        for (IndexT i = 0; i < mips; i++)
        {
            ResourceTableSetRWTexture(
                bloomState.resourceTable, {bloomState.intermediateBloomBufferViews[i], Bloom::BloomIntermediate::BINDING, i}
            );
        }
        ResourceTableSetTexture(bloomState.resourceTable, {bloomState.intermediateBloomTexture, Bloom::Intermediate::BINDING});
        ResourceTableSetTexture(bloomState.resourceTable, {bloomState.dirtMask, Bloom::DirtMask::BINDING});
        ResourceTableSetConstantBuffer(bloomState.resourceTable, {bloomState.constants, Bloom::BloomUniforms::BINDING});
        ResourceTableCommitChanges(bloomState.resourceTable);

        FrameScript_default::Bind_BloomIntermediate(Frame::TextureImport(bloomState.intermediateBloomTexture));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BloomContext::SetDirtMask(const CoreGraphics::TextureId texture)
{
    n_assert(texture != CoreGraphics::InvalidTextureId);
    if (bloomState.dirtMask == texture)
        return;
    bloomState.dirtMask = texture;
    WriteDirtUniform(texture);
    if (bloomState.resourceTable != CoreGraphics::InvalidResourceTableId)
    {
        CoreGraphics::ResourceTableSetTexture(bloomState.resourceTable, { texture, Bloom::DirtMask::BINDING });
        CoreGraphics::ResourceTableCommitChanges(bloomState.resourceTable);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BloomContext::Create()
{
    __CreatePluginContext();
    __bundle.OnViewportResized = BloomContext::Resize;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    using namespace CoreGraphics;
}

//------------------------------------------------------------------------------
/**
*/
void
BloomContext::Discard()
{
    DestroyResourceTable(bloomState.resourceTable);
}

} // namespace PostEffects
