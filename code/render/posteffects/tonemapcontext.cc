//------------------------------------------------------------------------------
//  tonemapcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "renderutil/drawfullscreenquad.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/resourcetable.h"
#include "graphics/graphicsserver.h"
#include "frame/frameplugin.h"
#include "tonemapcontext.h"
namespace PostEffects
{

__ImplementPluginContext(PostEffects::TonemapContext);

struct
{
    CoreGraphics::TextureId downsample2x2;
    CoreGraphics::TextureId copy;

    CoreGraphics::ShaderId shader;
    CoreGraphics::ResourceTableId tonemapTable;
    IndexT constantsSlot, colorSlot, prevSlot;

    CoreGraphics::ShaderProgramId program;

    IndexT timevar;
    CoreGraphics::BufferId constants;

    CoreGraphics::TextureId colorBuffer;
    CoreGraphics::TextureId averageLumBuffer;
} tonemapState;

//------------------------------------------------------------------------------
/**
*/
TonemapContext::TonemapContext()
{
}

//------------------------------------------------------------------------------
/**
*/
TonemapContext::~TonemapContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
TonemapContext::Create()
{
    __CreatePluginContext();

    using namespace CoreGraphics;

    // begin by copying and mipping down to a 2x2 texture
    Frame::AddCallback("Tonemap-Downsample", [](const IndexT frame, const IndexT bufferIndex)
        {
#if NEBULA_GRAPHICS_DEBUG
            CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_RED, "Tonemapping Downsample");
#endif
            BarrierInsert(
                GraphicsQueueType,
                BarrierStage::PixelShader,
                BarrierStage::Transfer,
                BarrierDomain::Global,
                {
                      TextureBarrier{ tonemapState.downsample2x2, ImageSubresourceInfo{ImageAspect::ColorBits, 0, 1, 0, 1}, ImageLayout::ShaderRead, ImageLayout::TransferDestination, BarrierAccess::ShaderRead, BarrierAccess::TransferWrite }
                },
                nullptr,
                "Tonemapping Downscale Begin");

            TextureDimensions dims = TextureGetDimensions(tonemapState.colorBuffer);
            Blit(tonemapState.colorBuffer, Math::rectangle<int>(0, 0, dims.width, dims.height), 0, 0, tonemapState.downsample2x2, Math::rectangle<int>(0, 0, 2, 2), 0, 0);

            BarrierInsert(
                GraphicsQueueType,
                BarrierStage::Transfer,
                BarrierStage::PixelShader,
                BarrierDomain::Global,
                {
                      TextureBarrier{ tonemapState.downsample2x2, ImageSubresourceInfo{ImageAspect::ColorBits, 0, 1, 0, 1}, ImageLayout::TransferDestination, ImageLayout::ShaderRead, BarrierAccess::TransferWrite, BarrierAccess::ShaderRead }
                },
                nullptr,
                "Tonemapping Downscale End");
#if NEBULA_GRAPHICS_DEBUG
            CommandBufferEndMarker(GraphicsQueueType);
#endif
        });

    // this pass calculates tonemapping from 2x2 cluster down to single pixel, called from the script
    Frame::AddCallback("Tonemap-AverageLum", [](const IndexT frame, const IndexT bufferIndex)
        {
#if NEBULA_GRAPHICS_DEBUG
            CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Tonemapping Average Luminance");
#endif

            Timing::Time time = FrameSync::FrameSyncTimer::Instance()->GetFrameTime();
            SetShaderProgram(tonemapState.program);
            BeginBatch(Frame::FrameBatchType::System);
            RenderUtil::DrawFullScreenQuad::ApplyMesh();
            BufferUpdate(tonemapState.constants, (float)time, tonemapState.timevar);
            SetResourceTable(tonemapState.tonemapTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);
            Draw();
            EndBatch();

#if NEBULA_GRAPHICS_DEBUG
            CommandBufferEndMarker(GraphicsQueueType);
#endif
        });

    // last pass, copy from render target to copy
    Frame::AddCallback("Tonemap-Copy", [](const IndexT frame, const IndexT bufferIndex)
        {
#if NEBULA_GRAPHICS_DEBUG
            CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_RED, "Tonemapping Copy Previous Frame");
#endif
            BarrierInsert(
                GraphicsQueueType,
                BarrierStage::PixelShader,
                BarrierStage::Transfer,
                BarrierDomain::Global,
                {
                      TextureBarrier{ tonemapState.copy, ImageSubresourceInfo::ColorNoMipNoLayer(), ImageLayout::ShaderRead, ImageLayout::TransferDestination, BarrierAccess::ShaderRead, BarrierAccess::TransferWrite }
                },
                nullptr,
                "Tonemapping Copy Last Frame Begin");

            CoreGraphics::TextureCopy from, to;
            from.region = Math::rectangle<int>(0, 0, 1, 1);
            from.mip = 0;
            from.layer = 0;
            to.region = Math::rectangle<int>(0, 0, 1, 1);
            to.mip = 0;
            to.layer = 0;
            Copy(GraphicsQueueType, tonemapState.averageLumBuffer, { from }, tonemapState.copy, { to });

            BarrierInsert(
                GraphicsQueueType,
                BarrierStage::Transfer,
                BarrierStage::PixelShader,
                BarrierDomain::Global,
                {
                      TextureBarrier{ tonemapState.copy, ImageSubresourceInfo::ColorNoMipNoLayer(), ImageLayout::TransferDestination, ImageLayout::ShaderRead, BarrierAccess::TransferWrite, BarrierAccess::ShaderRead }
                },
                nullptr,
                "Tonemapping Copy Last Frame End");

#if NEBULA_GRAPHICS_DEBUG
            CommandBufferEndMarker(GraphicsQueueType);
#endif
        });
}

//------------------------------------------------------------------------------
/**
*/
void 
TonemapContext::Discard()
{
    using namespace CoreGraphics;
    DestroyTexture(tonemapState.downsample2x2);
    DestroyTexture(tonemapState.copy);
    DestroyBuffer(tonemapState.constants);
    DestroyResourceTable(tonemapState.tonemapTable);
}

//------------------------------------------------------------------------------
/**
*/
void 
TonemapContext::Setup(const Ptr<Frame::FrameScript>& script)
{
    using namespace CoreGraphics;
    tonemapState.colorBuffer = script->GetTexture("LightBuffer");
    tonemapState.averageLumBuffer = script->GetTexture("AverageLumBuffer");

    TextureCreateInfo rtinfo;
    rtinfo.name = "Tonemapping-Downsample2x2"_atm;
    rtinfo.type = Texture2D;
    rtinfo.format = TextureGetPixelFormat(tonemapState.colorBuffer);
    rtinfo.width = 2;
    rtinfo.height = 2;
    rtinfo.usage = TextureUsage::TransferTextureDestination;
    tonemapState.downsample2x2 = CreateTexture(rtinfo);

    rtinfo.name = "Tonemapping-Copy";
    rtinfo.width = 1;
    rtinfo.height = 1;
    rtinfo.format = TextureGetPixelFormat(tonemapState.averageLumBuffer);
    rtinfo.usage = TextureUsage::TransferTextureDestination;
    tonemapState.copy = CreateTexture(rtinfo);

    // create shader
    tonemapState.shader = ShaderGet("shd:averagelum.fxb");
    tonemapState.tonemapTable = ShaderCreateResourceTable(tonemapState.shader, NEBULA_BATCH_GROUP);
    tonemapState.constants = ShaderCreateConstantBuffer(tonemapState.shader, "AverageLumBlock");
    tonemapState.timevar = ShaderGetConstantBinding(tonemapState.shader, "TimeDiff");
    tonemapState.colorSlot = ShaderGetResourceSlot(tonemapState.shader, "ColorSource");
    tonemapState.prevSlot = ShaderGetResourceSlot(tonemapState.shader, "PreviousLum");
    tonemapState.constantsSlot = ShaderGetResourceSlot(tonemapState.shader, "AverageLumBlock");
    tonemapState.program = ShaderGetProgram(tonemapState.shader, ShaderFeatureFromString("Alt0"));
    ResourceTableSetConstantBuffer(tonemapState.tonemapTable, { tonemapState.constants, tonemapState.constantsSlot, 0, false, false, -1, 0 });
    ResourceTableSetTexture(tonemapState.tonemapTable, { tonemapState.copy, tonemapState.prevSlot, 0, InvalidSamplerId, false });
    ResourceTableSetTexture(tonemapState.tonemapTable, { tonemapState.downsample2x2, tonemapState.colorSlot, 0, InvalidSamplerId, false });
    ResourceTableCommitChanges(tonemapState.tonemapTable);
}
} // namespace PostEffects
