//------------------------------------------------------------------------------
//  @file downsamplingcontext.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "frame/framesubgraph.h"
#include "downsamplingcontext.h"

#include "system_shaders/downsample/downsample_cs_light.h"
#include "system_shaders/downsample/downsample_cs_depth.h"
#include "system_shaders/downsample/depth_extract_cs.h"
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

    CoreGraphics::TextureId lightBuffer, depthBuffer, zbuffer;

    Ptr<Frame::FrameScript> frameScript;

    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 2> frameOpAllocator;

} state;
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
    , IndexT slot6
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
        info.bits = CoreGraphics::ImageBits::ColorBits;
        views[i] = CoreGraphics::CreateTextureView(info);

        if (i == 6)
        {
            CoreGraphics::ResourceTableSetRWTexture(table,
            {
                views[i],
                slot6,
                0,
            });
        }
        else
        {
            CoreGraphics::ResourceTableSetRWTexture(table,
            {
                views[i],
                slot,
                i,
            });
        }
    }
    CoreGraphics::ResourceTableCommitChanges(table);
}

//------------------------------------------------------------------------------
/**
*/
void
DownsamplingContext::Setup(const Ptr<Frame::FrameScript>& script)
{
    using namespace CoreGraphics;

    state.frameScript = script;
    state.downsampleColorShader = ShaderGet("shd:system_shaders/downsample/downsample_cs_light.fxb");
    state.downsampleColorProgram = ShaderGetProgram(state.downsampleColorShader, ShaderFeatureMask("Downsample"));
    state.downsampleDepthShader = ShaderGet("shd:system_shaders/downsample/downsample_cs_depth.fxb");
    state.downsampleDepthProgram = ShaderGetProgram(state.downsampleDepthShader, ShaderFeatureMask("Downsample"));

    state.extractShader = ShaderGet("shd:system_shaders/downsample/depth_extract_cs.fxb");
    state.extractProgram = ShaderGetProgram(state.extractShader, ShaderFeatureMask("Extract"));

    state.colorDownsampleResourceTable = ShaderCreateResourceTable(state.downsampleColorShader, NEBULA_BATCH_GROUP);
    state.depthDownsampleResourceTable = ShaderCreateResourceTable(state.downsampleDepthShader, NEBULA_BATCH_GROUP);
    state.extractResourceTable = ShaderCreateResourceTable(state.extractShader, NEBULA_BATCH_GROUP);

    state.lightBuffer = state.frameScript->GetTexture("LightBuffer");
    state.depthBuffer = state.frameScript->GetTexture("Depth");
    state.zbuffer = state.frameScript->GetTexture("ZBuffer");

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

    auto dims = TextureGetDimensions(state.lightBuffer);
    auto mips = TextureGetNumMips(state.lightBuffer);
    uint dispatchX = (dims.width - 1) / 64;
    uint dispatchY = (dims.height - 1) / 64;

    DownsampleCsLight::DownsampleUniforms constants;
    constants.Mips = mips - 1;
    constants.NumGroups = (dispatchX + 1) * (dispatchY + 1);
    BufferUpdate(state.colorBufferConstants, constants, 0);

    dims = TextureGetDimensions(state.depthBuffer);
    mips = TextureGetNumMips(state.depthBuffer);
    dispatchX = (dims.width - 1) / 64;
    dispatchY = (dims.height - 1) / 64;

    constants.Mips = mips - 1;
    constants.NumGroups = (dispatchX + 1) * (dispatchY + 1);
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
        state.zbuffer,
        DepthExtractCs::Table_Batch::ZBufferInput_SLOT,
        0,
        CoreGraphics::InvalidSamplerId,
        true,
        false
    });

    CoreGraphics::ResourceTableSetRWTexture(state.extractResourceTable, {
        state.depthBuffer,
        DepthExtractCs::Table_Batch::DepthOutput_SLOT
    });
    CoreGraphics::ResourceTableCommitChanges(state.extractResourceTable);

    Frame::FrameCode* colorDownsamplePass = state.frameOpAllocator.Alloc<Frame::FrameCode>();
    colorDownsamplePass->SetName("Color Downsample");
    colorDownsamplePass->domain = BarrierDomain::Global;
    colorDownsamplePass->textureDepRefs.Add(&state.lightBuffer,
                                        {
                                             "LightBuffer"
                                             , PipelineStage::ComputeShaderWrite
                                             , TextureSubresourceInfo::Color(state.lightBuffer)
                                        });

    colorDownsamplePass->func = [](const CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        TextureDimensions dims = TextureGetDimensions(state.lightBuffer);
        CmdSetShaderProgram(cmdBuf, state.downsampleColorProgram, false);
        CmdSetResourceTable(cmdBuf, state.colorDownsampleResourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = (dims.width - 1) / 64;
        uint dispatchY = (dims.height - 1) / 64;
        CmdDispatch(cmdBuf, dispatchX + 1, dispatchY + 1, 1);
    };

    Frame::AddSubgraph("Color Downsample", { colorDownsamplePass });

    Frame::FrameCode* depthDownsamplePass = state.frameOpAllocator.Alloc<Frame::FrameCode>();
    depthDownsamplePass->SetName("Depth Downsample");
    depthDownsamplePass->domain = BarrierDomain::Global;
    depthDownsamplePass->textureDepRefs.Add(&state.depthBuffer,
                                        {
                                             "DepthBuffer"
                                             , PipelineStage::ComputeShaderWrite
                                             , TextureSubresourceInfo::Color(state.depthBuffer)
                                        });

    depthDownsamplePass->func = [](const CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        TextureDimensions dims = TextureGetDimensions(state.depthBuffer);
        CmdSetShaderProgram(cmdBuf, state.downsampleDepthProgram, false);
        CmdSetResourceTable(cmdBuf, state.depthDownsampleResourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = (dims.width - 1) / 64;
        uint dispatchY = (dims.height - 1) / 64;
        CmdDispatch(cmdBuf, dispatchX + 1, dispatchY + 1, 1);
    };

    Frame::AddSubgraph("Depth Downsample", { depthDownsamplePass });

    Frame::FrameCode* extractPass = state.frameOpAllocator.Alloc<Frame::FrameCode>();
    extractPass->SetName("Depth Extract");
    extractPass->domain = BarrierDomain::Global;
    extractPass->textureDepRefs.Add(&state.depthBuffer,
                                 {
                                     "DepthBuffer"
                                     , PipelineStage::ComputeShaderWrite
                                     , TextureSubresourceInfo::Color(state.depthBuffer)
                                 });

    extractPass->textureDepRefs.Add(&state.zbuffer,
                                 {
                                     "ZBuffer"
                                     , PipelineStage::ComputeShaderRead
                                     , TextureSubresourceInfo::DepthStencil(state.zbuffer)
                                 });
    extractPass->func = [](const CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        TextureDimensions dims = TextureGetDimensions(state.depthBuffer);
        CmdSetShaderProgram(cmdBuf, state.extractProgram, false);
        CmdSetResourceTable(cmdBuf, state.extractResourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = (dims.width - 1) / 64;
        CmdDispatch(cmdBuf, dispatchX + 1, dims.height, 1);
    };

    Frame::AddSubgraph("Depth Extract", { extractPass });

    // Setup mip chains in resource tables
    SetupMipChainResources(state.lightBuffer, state.downsampledColorBufferViews, state.colorDownsampleResourceTable, "Color Downsample", false, DownsampleCsLight::Table_Batch::Output6_SLOT, DownsampleCsLight::Table_Batch::Output_SLOT);
    SetupMipChainResources(state.depthBuffer, state.downsampledDepthBufferViews, state.depthDownsampleResourceTable, "Depth Downsample", false, DownsampleCsDepth::Table_Batch::Output6_SLOT, DownsampleCsDepth::Table_Batch::Output_SLOT);
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

    state.lightBuffer = state.frameScript->GetTexture("LightBuffer");
    state.depthBuffer = state.frameScript->GetTexture("Depth");
    state.zbuffer = state.frameScript->GetTexture("ZBuffer");

    // Setup new views
    SetupMipChainResources(state.lightBuffer, state.downsampledColorBufferViews, state.colorDownsampleResourceTable, "Color Downsample", false, DownsampleCsLight::Table_Batch::Output6_SLOT, DownsampleCsLight::Table_Batch::Output_SLOT);
    SetupMipChainResources(state.depthBuffer, state.downsampledDepthBufferViews, state.depthDownsampleResourceTable, "Depth Downsample", true, DownsampleCsDepth::Table_Batch::Output6_SLOT, DownsampleCsDepth::Table_Batch::Output_SLOT);

    auto dims = TextureGetDimensions(state.lightBuffer);
    auto mips = TextureGetNumMips(state.lightBuffer);
    uint dispatchX = (dims.width - 1) / 64;
    uint dispatchY = (dims.height - 1) / 64;

    DownsampleCsLight::DownsampleUniforms constants;
    constants.Mips = mips - 1;
    constants.NumGroups = (dispatchX + 1) * (dispatchY + 1);
    BufferUpdate(state.colorBufferConstants, constants, 0);

    dims = TextureGetDimensions(state.depthBuffer);
    mips = TextureGetNumMips(state.depthBuffer);
    dispatchX = (dims.width - 1) / 64;
    dispatchY = (dims.height - 1) / 64;

    constants.Mips = mips - 1;
    constants.NumGroups = (dispatchX + 1) * (dispatchY + 1);
    BufferUpdate(state.depthBufferConstants, constants, 0);

    CoreGraphics::ResourceTableSetTexture(state.extractResourceTable, {
        state.zbuffer,
        DepthExtractCs::Table_Batch::ZBufferInput_SLOT,
        0,
        CoreGraphics::InvalidSamplerId,
        true,
        false
    });

    CoreGraphics::ResourceTableSetRWTexture(state.extractResourceTable, {
        state.depthBuffer,
        DepthExtractCs::Table_Batch::DepthOutput_SLOT
    });
    CoreGraphics::ResourceTableCommitChanges(state.extractResourceTable);
}

} // namespace PostEffects
