//------------------------------------------------------------------------------
//  @file downsamplingcontext.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "frame/framesubgraph.h"
#include "downsamplingcontext.h"

#include "render/system_shaders/downsample/downsample_cs_light.h"
#include "render/system_shaders/downsample/downsample_cs_depth.h"
#include "render/system_shaders/downsample/depth_extract_cs.h"

#include "frame/default.h"

namespace PostEffects
{


__ImplementPluginContext(DownsamplingContext);

struct
{
    CoreGraphics::ResourceTableId depthDownsampleResourceTable;
    CoreGraphics::ResourceTableId colorDownsampleResourceTable;

    CoreGraphics::ShaderId downsampleDepthShader;
    CoreGraphics::ShaderProgramId downsampleDepthProgram;
    CoreGraphics::ShaderId downsampleColorShader;
    CoreGraphics::ShaderProgramId downsampleColorProgram;

    CoreGraphics::ShaderId extractShader;
    CoreGraphics::ShaderProgramId extractProgram;
    CoreGraphics::ResourceTableId extractResourceTable;

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
    __bundle.OnWindowResized = DownsamplingContext::WindowResized;
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
*/
void
SetupMipChainResources(
    CoreGraphics::TextureId tex
    , Util::FixedArray<CoreGraphics::TextureViewId>& views
    , CoreGraphics::ResourceTableId table
    , const Util::String& name
    , bool depth
    , IndexT input
    , IndexT slot
)
{
    // Setup resources
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
    CoreGraphics::ResourceTableCommitChanges(table);
}

//------------------------------------------------------------------------------
/**
*/
void
DownsamplingContext::Setup()
{
    using namespace CoreGraphics;

    state.downsampleColorShader = ShaderGet("shd:system_shaders/downsample/downsample_cs_light.fxb");
    state.downsampleColorProgram = ShaderGetProgram(state.downsampleColorShader, ShaderFeatureMask("Downsample"));
    state.downsampleDepthShader = ShaderGet("shd:system_shaders/downsample/downsample_cs_depth.fxb");
    state.downsampleDepthProgram = ShaderGetProgram(state.downsampleDepthShader, ShaderFeatureMask("Downsample"));

    state.extractShader = ShaderGet("shd:system_shaders/downsample/depth_extract_cs.fxb");
    state.extractProgram = ShaderGetProgram(state.extractShader, ShaderFeatureMask("Extract"));

    state.colorDownsampleResourceTable = ShaderCreateResourceTable(state.downsampleColorShader, NEBULA_BATCH_GROUP);
    state.depthDownsampleResourceTable = ShaderCreateResourceTable(state.downsampleDepthShader, NEBULA_BATCH_GROUP);
    state.extractResourceTable = ShaderCreateResourceTable(state.extractShader, NEBULA_BATCH_GROUP);

 
    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.elementSize = sizeof(uint);
    bufInfo.size = 6;
    bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer;
    bufInfo.mode = CoreGraphics::DeviceLocal;
    bufInfo.queueSupport = CoreGraphics::ComputeQueueSupport;
    uint initData[6] = { 0 };
    bufInfo.data = &initData;
    bufInfo.dataSize = sizeof(initData);
    state.colorBufferCounter = CoreGraphics::CreateBuffer(bufInfo);
    state.depthBufferCounter = CoreGraphics::CreateBuffer(bufInfo);

    bufInfo.elementSize = sizeof(DownsampleCsDepth::DownsampleUniforms);
    bufInfo.mode = CoreGraphics::DeviceAndHost;
    bufInfo.usageFlags = CoreGraphics::ConstantBuffer;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;
    state.colorBufferConstants = CoreGraphics::CreateBuffer(bufInfo);
    state.depthBufferConstants = CoreGraphics::CreateBuffer(bufInfo);

    auto dims = TextureGetDimensions(FrameScript_default::Texture_LightBuffer());
    auto mips = TextureGetNumMips(FrameScript_default::Texture_LightBuffer());
    uint dispatchX = DispatchSize(dims.width);
    uint dispatchY = DispatchSize(dims.height);

    DownsampleCsLight::DownsampleUniforms constants;
    constants.Mips = mips - 1;
    constants.NumGroups = dispatchX * dispatchY;
    constants.Dimensions[0] = dims.width - 1;
    constants.Dimensions[1] = dims.height - 1;
    BufferUpdate(state.colorBufferConstants, constants, 0);

    dims = TextureGetDimensions(FrameScript_default::Texture_Depth());
    mips = TextureGetNumMips(FrameScript_default::Texture_Depth());
    dispatchX = DispatchSize(dims.width);
    dispatchY = DispatchSize(dims.height);

    constants.Mips = mips - 1;
    constants.NumGroups = dispatchX * dispatchY;
    constants.Dimensions[0] = dims.width - 1;
    constants.Dimensions[1] = dims.height - 1;
    BufferUpdate(state.depthBufferConstants, constants, 0);

    CoreGraphics::ResourceTableSetRWBuffer(state.colorDownsampleResourceTable, {
        state.colorBufferCounter,
        DownsampleCsLight::Table_Batch::AtomicCounter_SLOT,
    });

    CoreGraphics::ResourceTableSetConstantBuffer(state.colorDownsampleResourceTable, {
        state.colorBufferConstants,
        DownsampleCsLight::Table_Batch::DownsampleUniforms_SLOT,
    });

    CoreGraphics::ResourceTableSetRWBuffer(state.depthDownsampleResourceTable, {
        state.depthBufferCounter,
        DownsampleCsDepth::Table_Batch::AtomicCounter_SLOT,
    });

    CoreGraphics::ResourceTableSetConstantBuffer(state.depthDownsampleResourceTable, {
        state.depthBufferConstants,
        DownsampleCsDepth::Table_Batch::DownsampleUniforms_SLOT,
    });

    CoreGraphics::ResourceTableSetTexture(state.extractResourceTable, {
        FrameScript_default::Texture_ZBuffer(),
        DepthExtractCs::Table_Batch::ZBufferInput_SLOT,
        0,
        CoreGraphics::InvalidSamplerId,
        true,
        false
    });

    CoreGraphics::ResourceTableSetRWTexture(state.extractResourceTable, {
        FrameScript_default::Texture_Depth(),
        DepthExtractCs::Table_Batch::DepthOutput_SLOT
    });
    CoreGraphics::ResourceTableCommitChanges(state.extractResourceTable);

    FrameScript_default::RegisterSubgraph_DepthExtract_Compute([](const CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, state.extractProgram, false);
        CmdSetResourceTable(cmdBuf, state.extractResourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = Math::divandroundup(viewport.width(), 64);
        CmdDispatch(cmdBuf, dispatchX, viewport.height(), 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::Depth, PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::TextureIndex::ZBuffer, PipelineStage::ComputeShaderRead }
    });

    FrameScript_default::RegisterSubgraph_ColorDownsample_Compute([](const CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, state.downsampleColorProgram, false);
        CmdSetResourceTable(cmdBuf, state.colorDownsampleResourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = Math::divandroundup(viewport.width(), 64);
        uint dispatchY = Math::divandroundup(viewport.height(), 64);
        CmdDispatch(cmdBuf, dispatchX, dispatchY, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::LightBuffer, PipelineStage::ComputeShaderWrite }
    });

    FrameScript_default::RegisterSubgraph_DepthDownsample_Compute([](const CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, state.downsampleDepthProgram, false);
        CmdSetResourceTable(cmdBuf, state.depthDownsampleResourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = DispatchSize(viewport.width());
        uint dispatchY = DispatchSize(viewport.height());
        CmdDispatch(cmdBuf, dispatchX, dispatchY, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::Depth, PipelineStage::ComputeShaderWrite }
    });

    // Setup mip chains in resource tables
    SetupMipChainResources(FrameScript_default::Texture_LightBuffer(), state.downsampledColorBufferViews, state.colorDownsampleResourceTable, "Color Downsample", false, DownsampleCsLight::Table_Batch::Input_SLOT, DownsampleCsLight::Table_Batch::Output_SLOT);
    SetupMipChainResources(FrameScript_default::Texture_Depth(), state.downsampledDepthBufferViews, state.depthDownsampleResourceTable, "Depth Downsample", false, DownsampleCsDepth::Table_Batch::Input_SLOT, DownsampleCsDepth::Table_Batch::Output_SLOT);
}

//------------------------------------------------------------------------------
/**
*/
void
DownsamplingContext::WindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
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
    SetupMipChainResources(FrameScript_default::Texture_LightBuffer(), state.downsampledColorBufferViews, state.colorDownsampleResourceTable, "Color Downsample", false, DownsampleCsLight::Table_Batch::Input_SLOT, DownsampleCsLight::Table_Batch::Output_SLOT);
    SetupMipChainResources(FrameScript_default::Texture_Depth(), state.downsampledDepthBufferViews, state.depthDownsampleResourceTable, "Depth Downsample", false, DownsampleCsDepth::Table_Batch::Input_SLOT, DownsampleCsDepth::Table_Batch::Output_SLOT);

    auto dims = TextureGetDimensions(FrameScript_default::Texture_LightBuffer());
    auto mips = TextureGetNumMips(FrameScript_default::Texture_LightBuffer());
    uint dispatchX = DispatchSize(dims.width);
    uint dispatchY = DispatchSize(dims.height);

    DownsampleCsLight::DownsampleUniforms constants;
    constants.Mips = mips - 1;
    constants.NumGroups = dispatchX * dispatchY;
    constants.Dimensions[0] = dims.width - 1;
    constants.Dimensions[1] = dims.height - 1;
    BufferUpdate(state.colorBufferConstants, constants, 0);

    dims = TextureGetDimensions(FrameScript_default::Texture_Depth());
    mips = TextureGetNumMips(FrameScript_default::Texture_Depth());
    dispatchX = DispatchSize(dims.width);
    dispatchY = DispatchSize(dims.height);

    constants.Mips = mips - 1;
    constants.NumGroups = dispatchX * dispatchY;
    constants.Dimensions[0] = dims.width - 1;
    constants.Dimensions[1] = dims.height - 1;
    BufferUpdate(state.depthBufferConstants, constants, 0);

    CoreGraphics::ResourceTableSetTexture(state.extractResourceTable, {
        FrameScript_default::Texture_ZBuffer(),
        DepthExtractCs::Table_Batch::ZBufferInput_SLOT,
        0,
        CoreGraphics::InvalidSamplerId,
        true,
        false
    });

    CoreGraphics::ResourceTableSetRWTexture(state.extractResourceTable, {
        FrameScript_default::Texture_Depth(),
        DepthExtractCs::Table_Batch::DepthOutput_SLOT
    });
    CoreGraphics::ResourceTableCommitChanges(state.extractResourceTable);
}

} // namespace PostEffects
