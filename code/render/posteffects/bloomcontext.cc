//------------------------------------------------------------------------------
//  bloomcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frame/frameplugin.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/resourcetable.h"
#include "graphics/graphicsserver.h"
#include "renderutil/drawfullscreenquad.h"
#include "bloomcontext.h"

#include "blur_bloom.h"
namespace PostEffects
{

__ImplementPluginContext(PostEffects::BloomContext);
struct
{
    CoreGraphics::TextureId internalTargets[1];
    CoreGraphics::ShaderProgramId brightPassProgram;
    CoreGraphics::ShaderProgramId blurX, blurY;
    CoreGraphics::ShaderId brightPassShader, blurShader;

    CoreGraphics::ResourceTableId brightPassTable, blurTable;
    IndexT colorSourceSlot, luminanceTextureSlot, inputImageXSlot, inputImageYSlot, blurImageXSlot, blurImageYSlot;

    CoreGraphics::TextureId blurredBloom;
    CoreGraphics::TextureId lightBuffer;

} bloomState;

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
BloomContext::Setup(const Ptr<Frame::FrameScript>& script)
{
    using namespace CoreGraphics;

    // setup shaders
    bloomState.brightPassShader = ShaderGet("shd:brightpass.fxb");
    bloomState.blurShader = ShaderGet("shd:blur_bloom.fxb");
    bloomState.brightPassTable = ShaderCreateResourceTable(bloomState.brightPassShader, NEBULA_BATCH_GROUP);
    bloomState.blurTable = ShaderCreateResourceTable(bloomState.blurShader, NEBULA_BATCH_GROUP);

    bloomState.colorSourceSlot = ShaderGetResourceSlot(bloomState.brightPassShader, "ColorSource");
    bloomState.inputImageXSlot = ShaderGetResourceSlot(bloomState.blurShader, "InputImageX");
    bloomState.inputImageYSlot = ShaderGetResourceSlot(bloomState.blurShader, "InputImageY");
    bloomState.blurImageXSlot = ShaderGetResourceSlot(bloomState.blurShader, "BlurImageX");
    bloomState.blurImageYSlot = ShaderGetResourceSlot(bloomState.blurShader, "BlurImageY");

    bloomState.blurredBloom = script->GetTexture("BloomBufferBlurred");
    bloomState.lightBuffer = script->GetTexture("LightBuffer");
    CoreGraphics::TextureRelativeDimensions relDims = CoreGraphics::TextureGetRelativeDimensions(bloomState.blurredBloom);

    TextureCreateInfo tinfo;
    tinfo.name = "Bloom-Internal0";
    tinfo.type = Texture2D;
    tinfo.format = CoreGraphics::PixelFormat::R16G16B16A16F;
    tinfo.width = relDims.width;
    tinfo.height = relDims.height;
    tinfo.usage = TextureUsage::ReadWriteTexture;
    tinfo.windowRelative = true;
    bloomState.internalTargets[0] = CreateTexture(tinfo);

    ResourceTableSetTexture(bloomState.brightPassTable, { bloomState.lightBuffer, bloomState.colorSourceSlot, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableCommitChanges(bloomState.brightPassTable);

    // bloom buffer goes in, internal target goes out
    ResourceTableSetTexture(bloomState.blurTable, { bloomState.blurredBloom, bloomState.inputImageXSlot, 0, CoreGraphics::InvalidSamplerId , false });
    ResourceTableSetRWTexture(bloomState.blurTable, { bloomState.internalTargets[0], bloomState.blurImageXSlot, 0, CoreGraphics::InvalidSamplerId });

    // internal target goes in, blurred buffer goes out
    ResourceTableSetTexture(bloomState.blurTable, { bloomState.internalTargets[0], bloomState.inputImageYSlot, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableSetRWTexture(bloomState.blurTable, { bloomState.blurredBloom, bloomState.blurImageYSlot, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableCommitChanges(bloomState.blurTable);

    bloomState.blurX = ShaderGetProgram(bloomState.blurShader, ShaderFeatureFromString("Alt0"));
    bloomState.blurY = ShaderGetProgram(bloomState.blurShader, ShaderFeatureFromString("Alt1"));
    bloomState.brightPassProgram = ShaderGetProgram(bloomState.brightPassShader, ShaderFeatureFromString("Alt0"));
}

//------------------------------------------------------------------------------
/**
*/
void
BloomContext::WindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
{
    using namespace CoreGraphics;
    DestroyTexture(bloomState.internalTargets[0]);

    CoreGraphics::TextureRelativeDimensions relDims = CoreGraphics::TextureGetRelativeDimensions(bloomState.blurredBloom);
    TextureCreateInfo tinfo;
    tinfo.name = "Bloom-Internal0";
    tinfo.type = Texture2D;
    tinfo.format = CoreGraphics::PixelFormat::R16G16B16A16F;
    tinfo.width = relDims.width;
    tinfo.height = relDims.height;
    tinfo.usage = TextureUsage::ReadWriteTexture;
    tinfo.windowRelative = true;
    bloomState.internalTargets[0] = CreateTexture(tinfo);

    ResourceTableSetTexture(bloomState.brightPassTable, { bloomState.lightBuffer, bloomState.colorSourceSlot, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableCommitChanges(bloomState.brightPassTable);

    // bloom buffer goes in, internal target goes out
    ResourceTableSetTexture(bloomState.blurTable, { bloomState.blurredBloom, bloomState.inputImageXSlot, 0, CoreGraphics::InvalidSamplerId, false });
    ResourceTableSetRWTexture(bloomState.blurTable, { bloomState.internalTargets[0], bloomState.blurImageXSlot, 0, CoreGraphics::InvalidSamplerId });

    // internal target goes in, blurred buffer goes out
    ResourceTableSetTexture(bloomState.blurTable, { bloomState.internalTargets[0], bloomState.inputImageYSlot, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableSetRWTexture(bloomState.blurTable, { bloomState.blurredBloom, bloomState.blurImageYSlot, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableCommitChanges(bloomState.blurTable);
}

//------------------------------------------------------------------------------
/**
*/
void 
BloomContext::Create()
{
    __CreatePluginContext();
    __bundle.OnWindowResized = BloomContext::WindowResized;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    using namespace CoreGraphics;
    Frame::AddCallback("Bloom-BrightnessLowpass", [](const IndexT frame, const IndexT bufferIndex)
        {
#if NEBULA_GRAPHICS_DEBUG
            CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_ORANGE, "BrightnessLowpass");
#endif
            CoreGraphics::SetShaderProgram(bloomState.brightPassProgram);
            CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
            RenderUtil::DrawFullScreenQuad::ApplyMesh();
            CoreGraphics::SetResourceTable(bloomState.brightPassTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
            CoreGraphics::Draw();
            CoreGraphics::EndBatch();
#if NEBULA_GRAPHICS_DEBUG
            CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
        });

    Frame::AddCallback("Bloom-Blur", [](const IndexT frame, const IndexT bufferIndex)
        {
#if NEBULA_GRAPHICS_DEBUG
            CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "BloomBlur");
#endif
            TextureDimensions dims = TextureGetDimensions(bloomState.internalTargets[0]);

            // calculate execution dimensions
            uint numGroupsX1 = Math::divandroundup(dims.width, BlurBloom::BlurTileWidth);
            uint numGroupsX2 = dims.width;
            uint numGroupsY1 = Math::divandroundup(dims.height, BlurBloom::BlurTileWidth);
            uint numGroupsY2 = dims.height;

            // do 5 bloom steps
            for (int i = 0; i < 5; i++)
            {
                CoreGraphics::BarrierInsert(
                    GraphicsQueueType,
                    CoreGraphics::BarrierStage::ComputeShader,
                    CoreGraphics::BarrierStage::ComputeShader,
                    CoreGraphics::BarrierDomain::Global,
                    {
                          TextureBarrier{ bloomState.internalTargets[0], ImageSubresourceInfo::ColorNoMipNoLayer(), CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::General, CoreGraphics::BarrierAccess::ShaderRead, CoreGraphics::BarrierAccess::ShaderWrite }
                    },
                    nullptr,
                    "Bloom Blur Pass #1 Begin");

                CoreGraphics::SetShaderProgram(bloomState.blurX);
                CoreGraphics::SetResourceTable(bloomState.blurTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
                CoreGraphics::Compute(numGroupsX1, numGroupsY2, 1);

                CoreGraphics::BarrierInsert(
                    GraphicsQueueType,
                    CoreGraphics::BarrierStage::ComputeShader,
                    CoreGraphics::BarrierStage::ComputeShader,
                    CoreGraphics::BarrierDomain::Global,
                    {
                          TextureBarrier{ bloomState.internalTargets[0], ImageSubresourceInfo::ColorNoMipNoLayer(), CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::BarrierAccess::ShaderWrite, CoreGraphics::BarrierAccess::ShaderRead },
                          TextureBarrier{ bloomState.blurredBloom, ImageSubresourceInfo::ColorNoMipNoLayer(), CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::General, CoreGraphics::BarrierAccess::ShaderRead, CoreGraphics::BarrierAccess::ShaderWrite }
                    },
                    nullptr,
                    "Bloom Blur Pass #2 Mid");

                CoreGraphics::SetShaderProgram(bloomState.blurY);
                CoreGraphics::SetResourceTable(bloomState.blurTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
                CoreGraphics::Compute(numGroupsY1, numGroupsX2, 1);

                CoreGraphics::BarrierInsert(
                    GraphicsQueueType,
                    CoreGraphics::BarrierStage::ComputeShader,
                    CoreGraphics::BarrierStage::PixelShader,
                    CoreGraphics::BarrierDomain::Global,
                    {
                          TextureBarrier{ bloomState.blurredBloom, ImageSubresourceInfo::ColorNoMipNoLayer(), CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::BarrierAccess::ShaderWrite, CoreGraphics::BarrierAccess::ShaderRead }
                    },
                    nullptr,
                    "Bloom Blur Pass #2 End");
            }

#if NEBULA_GRAPHICS_DEBUG
            CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
        });
}

//------------------------------------------------------------------------------
/**
*/
void 
BloomContext::Discard()
{
    DestroyResourceTable(bloomState.brightPassTable);
    DestroyResourceTable(bloomState.blurTable);
    DestroyTexture(bloomState.internalTargets[0]);
}

} // namespace PostEffects
