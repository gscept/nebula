//------------------------------------------------------------------------------
//  @file downsamplingcontext.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "frame/framesubgraph.h"
#include "downsamplingcontext.h"

#include "gpulang/render/system_shaders/downsample/downsample_cs_light.h"
#include "gpulang/render/system_shaders/downsample/downsample_cs_depth.h"
#include "gpulang/render/system_shaders/downsample/depth_extract_cs.h"

#include "frame/default.h"

namespace PostEffects
{


__ImplementPluginContext(DownsamplingContext);

struct
{
    // One table per buffered frame each, so a resize never rewrites a table a frame on the GPU still uses
    CoreGraphics::ResourceTableSet depthDownsampleResourceTables;
    CoreGraphics::ResourceTableSet colorDownsampleResourceTables;

    CoreGraphics::ShaderId downsampleDepthShader;
    CoreGraphics::ShaderProgramId downsampleDepthProgram;
    CoreGraphics::ShaderId downsampleColorShader;
    CoreGraphics::ShaderProgramId downsampleColorProgram;

    CoreGraphics::ShaderId extractShader;
    CoreGraphics::ShaderProgramId extractProgram;
    CoreGraphics::ResourceTableSet extractResourceTables;

    Util::FixedArray<CoreGraphics::TextureViewId> downsampledColorBufferViews;
    CoreGraphics::BufferId colorBufferCounter;
    CoreGraphics::BufferId colorBufferConstants;
    Util::FixedArray<CoreGraphics::TextureViewId> downsampledDepthBufferViews;
    CoreGraphics::BufferId depthBufferCounter;
    CoreGraphics::BufferId depthBufferConstants;

} state;

//------------------------------------------------------------------------------
/**
*/
uint
DispatchSize(SizeT numPixels)
{
    return ((numPixels - 1) / 64) + 1;
}

//------------------------------------------------------------------------------
/**
    The constants are one buffer for all frames, so the update waits for the previous frame's dispatch
    to be done reading them, and the dispatch waits for the update
*/
template <typename CONSTANTS>
void
UpdateDownsampleConstants(const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::BufferId buffer, const CONSTANTS& constants)
{
    using namespace CoreGraphics;
    const BufferSubresourceInfo range(0, sizeof(CONSTANTS));
    CmdBarrier(cmdBuf, PipelineStage::UniformCompute, PipelineStage::TransferWrite, BarrierDomain::Global, { BufferBarrierInfo{ .buf = buffer, .subres = range } });
    CmdUpdateBuffer(cmdBuf, buffer, 0, sizeof(CONSTANTS), &constants);
    CmdBarrier(cmdBuf, PipelineStage::TransferWrite, PipelineStage::UniformCompute, BarrierDomain::Global, { BufferBarrierInfo{ .buf = buffer, .subres = range } });
}

//------------------------------------------------------------------------------
/**
*/
DownsamplingContext::DownsamplingContext()
{
}

//------------------------------------------------------------------------------
/**
*/
DownsamplingContext::~DownsamplingContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void
DownsamplingContext::Create()
{
    __CreatePluginContext();
    __bundle.OnViewportResized = DownsamplingContext::Resize;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
}

//------------------------------------------------------------------------------
/**
*/
void
DownsamplingContext::Discard()
{
    // Destroy old views
    for (auto& view : state.downsampledColorBufferViews)
    {
        CoreGraphics::DestroyTextureView(view);
    }
    for (auto& view : state.downsampledDepthBufferViews)
    {
        CoreGraphics::DestroyTextureView(view);
    }
}

//------------------------------------------------------------------------------
/**
    Creates one view per mip of the texture
*/
static void
CreateMipChainViews(
    CoreGraphics::TextureId tex
    , Util::FixedArray<CoreGraphics::TextureViewId>& views
    , const Util::String& name
    , bool depth
)
{
    auto mips = TextureGetNumMips(tex);
    views.Resize(mips);
    for (IndexT i = 0; i < mips; i++)
    {
        CoreGraphics::TextureViewCreateInfo info;
        info.name = Util::String::Sprintf("%s %d", name.AsCharPtr(), i);
        info.tex = tex;
        info.startMip = i;
        info.numMips = 1;
        info.format = CoreGraphics::TextureGetPixelFormat(tex);
        info.bits = depth ? CoreGraphics::ImageBits::DepthBits : CoreGraphics::ImageBits::ColorBits;
        views[i] = CoreGraphics::CreateTextureView(info);
    }
}

//------------------------------------------------------------------------------
/**
    Sets the mip views in a table, mip 0 is the input and the others are the outputs
*/
static void
SetMipChainViews(
    const CoreGraphics::ResourceTableId table
    , const Util::FixedArray<CoreGraphics::TextureViewId>& views
    , IndexT input
    , IndexT slot
)
{
    for (IndexT i = 0; i < views.Size(); i++)
    {
        if (i == 0)
        {
            CoreGraphics::ResourceTableSetRWTexture(table,
            {
                views[i],
                input,
                0,
            });
        }
        else
        {
            CoreGraphics::ResourceTableSetRWTexture(table,
            {
                views[i],
                slot,
                i - 1,
            });
        }
    }
}

//------------------------------------------------------------------------------
/**
    Writes one of the color downsample tables, with the views of the current size
*/
static void
WriteColorDownsampleTable(const CoreGraphics::ResourceTableId table)
{
    CoreGraphics::ResourceTableSetRWBuffer(table, {
        state.colorBufferCounter,
        DownsampleCsLight::Counters::BINDING,
    });
    CoreGraphics::ResourceTableSetConstantBuffer(table, {
        state.colorBufferConstants,
        DownsampleCsLight::DownsampleUniforms::BINDING,
    });
    SetMipChainViews(table, state.downsampledColorBufferViews, DownsampleCsLight::Input::BINDING, DownsampleCsLight::Output::BINDING);
    CoreGraphics::ResourceTableCommitChanges(table);
}

//------------------------------------------------------------------------------
/**
    Writes one of the depth downsample tables, with the views of the current size
*/
static void
WriteDepthDownsampleTable(const CoreGraphics::ResourceTableId table)
{
    CoreGraphics::ResourceTableSetRWBuffer(table, {
        state.depthBufferCounter,
        DownsampleCsDepth::Counters::BINDING,
    });
    CoreGraphics::ResourceTableSetConstantBuffer(table, {
        state.depthBufferConstants,
        DownsampleCsDepth::DownsampleUniforms::BINDING,
    });
    SetMipChainViews(table, state.downsampledDepthBufferViews, DownsampleCsDepth::Input::BINDING, DownsampleCsDepth::Output::BINDING);
    CoreGraphics::ResourceTableCommitChanges(table);
}

//------------------------------------------------------------------------------
/**
    Writes one of the depth extract tables, with the depth buffers of the current size
*/
static void
WriteExtractTable(const CoreGraphics::ResourceTableId table)
{
    CoreGraphics::ResourceTableSetTexture(table, {
        FrameScript_default::Texture_ZBuffer(),
        DepthExtractCs::ZBufferInput::BINDING,
        0,
        CoreGraphics::InvalidSamplerId,
        true,
        false
    });
    CoreGraphics::ResourceTableSetRWTexture(table, {
        FrameScript_default::Texture_Depth(),
        DepthExtractCs::DepthOutput::BINDING
    });
    CoreGraphics::ResourceTableCommitChanges(table);
}

//------------------------------------------------------------------------------
/**
*/
void
DownsamplingContext::Setup()
{
    using namespace CoreGraphics;

    state.downsampleColorShader = ShaderGet("shd:system_shaders/downsample/downsample_cs_light.gplb");
    state.downsampleColorProgram = ShaderGetProgram(state.downsampleColorShader, ShaderFeatureMask("Downsample"));
    state.downsampleDepthShader = ShaderGet("shd:system_shaders/downsample/downsample_cs_depth.gplb");
    state.downsampleDepthProgram = ShaderGetProgram(state.downsampleDepthShader, ShaderFeatureMask("Downsample"));

    state.extractShader = ShaderGet("shd:system_shaders/downsample/depth_extract_cs.gplb");
    state.extractProgram = ShaderGetProgram(state.extractShader, ShaderFeatureMask("Extract"));

    state.colorDownsampleResourceTables = ShaderCreateResourceTableSet(state.downsampleColorShader, NEBULA_BATCH_GROUP);
    state.colorDownsampleResourceTables.SetUpdate(WriteColorDownsampleTable);
    state.depthDownsampleResourceTables = ShaderCreateResourceTableSet(state.downsampleDepthShader, NEBULA_BATCH_GROUP);
    state.depthDownsampleResourceTables.SetUpdate(WriteDepthDownsampleTable);
    state.extractResourceTables = ShaderCreateResourceTableSet(state.extractShader, NEBULA_BATCH_GROUP);
    state.extractResourceTables.SetUpdate(WriteExtractTable);

 
    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.elementSize = sizeof(uint);
    bufInfo.size = 6;
    bufInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite;
    bufInfo.mode = CoreGraphics::DeviceLocal;
    bufInfo.queueSupport = CoreGraphics::ComputeQueueSupport;
    uint initData[6] = { 0 };
    bufInfo.data = &initData;
    bufInfo.dataSize = sizeof(initData);
    state.colorBufferCounter = CoreGraphics::CreateBuffer(bufInfo);
    state.depthBufferCounter = CoreGraphics::CreateBuffer(bufInfo);

    bufInfo.elementSize = sizeof(DownsampleCsDepth::DownsampleUniforms::STRUCT);
    bufInfo.mode = CoreGraphics::DeviceAndHost;
    bufInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer | CoreGraphics::BufferUsage::TransferDestination;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;
    state.colorBufferConstants = CoreGraphics::CreateBuffer(bufInfo);
    state.depthBufferConstants = CoreGraphics::CreateBuffer(bufInfo);

    auto dims = TextureGetDimensions(FrameScript_default::Texture_LightBuffer());
    auto mips = TextureGetNumMips(FrameScript_default::Texture_LightBuffer());
    uint dispatchX = DispatchSize(dims.width);
    uint dispatchY = DispatchSize(dims.height);

    // Each table is written when its frame is first recorded
    state.extractResourceTables.Invalidate();

    FrameScript_default::RegisterSubgraph_DepthExtract_Compute([](const CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, state.extractProgram, queue, false);
        CmdSetResourceTable(cmdBuf, state.extractResourceTables.Get(bufferIndex), NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = Math::divandroundup(viewport.width(), 64);
        CmdDispatch(cmdBuf, dispatchX, viewport.height(), 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::Depth, PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::TextureIndex::ZBuffer, PipelineStage::ComputeShaderRead }
    });

    FrameScript_default::RegisterSubgraph_ColorDownsample_Compute([](const CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, state.downsampleColorProgram, queue, false);
        CmdSetResourceTable(cmdBuf, state.colorDownsampleResourceTables.Get(bufferIndex), NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = Math::divandroundup(viewport.width(), 64);
        uint dispatchY = Math::divandroundup(viewport.height(), 64);

        DownsampleCsLight::DownsampleUniforms::STRUCT constants;
        // Number of mips the shader writes, the views for them are set up from the texture's own mip count
        constants.Mips = CoreGraphics::TextureGetNumMips(FrameScript_default::Texture_LightBuffer()) - 1;
        constants.NumGroups = dispatchX * dispatchY;
        for (IndexT i = 0; i < 16; i++)
        {
            constants.Dimensions[i][0] = Math::max(1, viewport.width() >> i);
            constants.Dimensions[i][1] = Math::max(1, viewport.height() >> i);
        }
        UpdateDownsampleConstants(cmdBuf, state.colorBufferConstants, constants);

        CmdDispatch(cmdBuf, dispatchX, dispatchY, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::LightBuffer, PipelineStage::ComputeShaderWrite }
    });

    FrameScript_default::RegisterSubgraph_DepthDownsample_Compute([](const CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, state.downsampleDepthProgram, queue, false);
        CmdSetResourceTable(cmdBuf, state.depthDownsampleResourceTables.Get(bufferIndex), NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = Math::divandroundup(viewport.width(), 64);
        uint dispatchY = Math::divandroundup(viewport.height(), 64);

        DownsampleCsLight::DownsampleUniforms::STRUCT constants;
        // See ColorDownsample_Compute
        constants.Mips = CoreGraphics::TextureGetNumMips(FrameScript_default::Texture_Depth()) - 1;
        constants.NumGroups = dispatchX * dispatchY;
        for (IndexT i = 0; i < 16; i++)
        {
            constants.Dimensions[i][0] = Math::max(1, viewport.width() >> i);
            constants.Dimensions[i][1] = Math::max(1, viewport.height() >> i);
        }
        UpdateDownsampleConstants(cmdBuf, state.depthBufferConstants, constants);

        CmdDispatch(cmdBuf, dispatchX, dispatchY, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::Depth, PipelineStage::ComputeShaderWrite }
    });

    // Setup mip chains, the tables get them when they are written
    CreateMipChainViews(FrameScript_default::Texture_LightBuffer(), state.downsampledColorBufferViews, "Color Downsample", false);
    CreateMipChainViews(FrameScript_default::Texture_Depth(), state.downsampledDepthBufferViews, "Depth Downsample", false);
    state.colorDownsampleResourceTables.Invalidate();
    state.depthDownsampleResourceTables.Invalidate();
}

//------------------------------------------------------------------------------
/**
*/
void
DownsamplingContext::Resize(const uint framescriptHash, SizeT width, SizeT height)
{
    if (framescriptHash == FrameScript_default::ID)
    {
        // Destroy old views
        for (auto& view : state.downsampledColorBufferViews)
        {
            CoreGraphics::DestroyTextureView(view);
        }
        for (auto& view : state.downsampledDepthBufferViews)
        {
            CoreGraphics::DestroyTextureView(view);
        }

        // Setup new views
        CreateMipChainViews(FrameScript_default::Texture_LightBuffer(), state.downsampledColorBufferViews, "Color Downsample", false);
        CreateMipChainViews(FrameScript_default::Texture_Depth(), state.downsampledDepthBufferViews, "Depth Downsample", false);

        // Frames still on the GPU keep their tables, which point to the old views until those are deleted.
        // This frame's tables are written now, before the frame is submitted, the others when their frames come around
        const IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();
        state.colorDownsampleResourceTables.Invalidate();
        state.colorDownsampleResourceTables.Get(bufferIndex);
        state.depthDownsampleResourceTables.Invalidate();
        state.depthDownsampleResourceTables.Get(bufferIndex);
        state.extractResourceTables.Invalidate();
        state.extractResourceTables.Get(bufferIndex);
    }
}

} // namespace PostEffects
