//------------------------------------------------------------------------------
//  bloomcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frame/framesubgraph.h"
#include "frame/framecode.h"
#include "frame/frameblit.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/resourcetable.h"
#include "graphics/graphicsserver.h"
#include "renderutil/drawfullscreenquad.h"
#include "bloomcontext.h"

#include "brightpass.h"
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

    CoreGraphics::TextureId bloomBuffer;
    CoreGraphics::TextureId blurredBloom;
    CoreGraphics::TextureId lightBuffer;

    CoreGraphics::TextureViewId bloomBufferView;
    CoreGraphics::PassId bloomPass;

    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 3 + sizeof(Frame::FrameBlit) + sizeof(Frame::FramePass) + sizeof(Frame::FrameSubpass)> frameOpAllocator;

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

    bloomState.bloomBuffer = script->GetTexture("BloomBuffer");
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

    ResourceTableSetTexture(bloomState.brightPassTable, { bloomState.lightBuffer, Brightpass::Table_Batch::ColorSource_SLOT, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableCommitChanges(bloomState.brightPassTable);

    // bloom buffer goes in, internal target goes out
    ResourceTableSetTexture(bloomState.blurTable, { bloomState.blurredBloom, BlurBloom::Table_Batch::InputImageX_SLOT, 0, CoreGraphics::InvalidSamplerId , false });
    ResourceTableSetRWTexture(bloomState.blurTable, { bloomState.internalTargets[0], BlurBloom::Table_Batch::BlurImageX_SLOT, 0, CoreGraphics::InvalidSamplerId });

    // internal target goes in, blurred buffer goes out
    ResourceTableSetTexture(bloomState.blurTable, { bloomState.internalTargets[0], BlurBloom::Table_Batch::InputImageY_SLOT, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableSetRWTexture(bloomState.blurTable, { bloomState.blurredBloom, BlurBloom::Table_Batch::BlurImageY_SLOT, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableCommitChanges(bloomState.blurTable);

    bloomState.blurX = ShaderGetProgram(bloomState.blurShader, ShaderFeatureFromString("Alt0"));
    bloomState.blurY = ShaderGetProgram(bloomState.blurShader, ShaderFeatureFromString("Alt1"));
    bloomState.brightPassProgram = ShaderGetProgram(bloomState.brightPassShader, ShaderFeatureFromString("Alt0"));

    // Create subpass
    CoreGraphics::Subpass subpass;
    subpass.attachments.Append(0);
    subpass.bindDepth = false;
    subpass.numScissors = 1;
    subpass.numViewports = 1;

    // Create pass
    CoreGraphics::PassCreateInfo passInfo;
    passInfo.name = "Bloom Pass";
    bloomState.bloomBufferView = CoreGraphics::CreateTextureView({ bloomState.bloomBuffer, 0, 1, 0, 1, TextureGetPixelFormat(bloomState.bloomBuffer) });
    passInfo.colorAttachments.Append(bloomState.bloomBufferView);
    passInfo.colorAttachmentFlags.Append(CoreGraphics::AttachmentFlagBits::Store);
    passInfo.colorAttachmentClears.Append(Math::vec4(1)); // dummy value
    passInfo.subpasses.Append(subpass);
    bloomState.bloomPass = CoreGraphics::CreatePass(passInfo);

    // Create subgraph for low pass
    Frame::FramePass* bloomPass = bloomState.frameOpAllocator.Alloc<Frame::FramePass>();
    bloomPass->SetName("Bloom");
    bloomPass->pass = bloomState.bloomPass;

    Frame::FrameSubpass* bloomSubpass = bloomState.frameOpAllocator.Alloc<Frame::FrameSubpass>();
    bloomSubpass->domain = BarrierDomain::Pass;
    bloomSubpass->SetName("Bloom Lowpass");
    bloomPass->AddChild(bloomSubpass);

    Frame::FrameCode* lowpassOp = bloomState.frameOpAllocator.Alloc<Frame::FrameCode>();
    lowpassOp->SetName("Bloom Brightness Lowpass");
    lowpassOp->domain = BarrierDomain::Pass;
    lowpassOp->textureDeps.Add(bloomState.lightBuffer,
                               {
                                   "LightBuffer"
                                   , CoreGraphics::PipelineStage::PixelShaderRead
                                   , CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                               });

    lowpassOp->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_ORANGE, "Brightness Lowpass Filter");

        CoreGraphics::CmdSetShaderProgram(cmdBuf, bloomState.brightPassProgram);
        RenderUtil::DrawFullScreenQuad::ApplyMesh(cmdBuf);
        CoreGraphics::CmdSetResourceTable(cmdBuf, bloomState.brightPassTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
        CoreGraphics::CmdDraw(cmdBuf, RenderUtil::DrawFullScreenQuad::GetPrimitiveGroup());
    };

    // Add the code node to the subpass, this is how we actually render
    bloomSubpass->AddChild(lowpassOp);

    // Create an array of frame operations
    Util::Array<Frame::FrameOp*> pluginOps;

    // Finally add the pass to the subgraph
    pluginOps.Append(bloomPass);

    // The hierarchy is:
    // Bloom Pass
    // |
    // +-- Bloom Subpass
    //   |
    //   | -- Bloom Lowpass Op -> BloomBuffer
    //   |
    // |
    // |-- Blit BloomBuffer -> BlurredBloomBuffer
    // |
    // |-- Bloom Blur (BlurredBloomBuffer -> BloomBuffer, BloomBuffer -> BlurredBloomBuffer) x 5

    // Create pass for bloom blit
    Frame::FrameBlit* blit = bloomState.frameOpAllocator.Alloc<Frame::FrameBlit>();
    blit->SetName("Bloom Blit");
    blit->textureDeps.Add(bloomState.bloomBuffer,
                    {
                        "BloomBuffer",
                        CoreGraphics::PipelineStage::TransferRead,
                        CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                    });
    blit->textureDeps.Add(bloomState.blurredBloom,
                    {
                        "BloomBufferBlurred",
                        CoreGraphics::PipelineStage::TransferWrite,
                        CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                    });
    blit->from = bloomState.bloomBuffer;
    blit->to = bloomState.blurredBloom;
    pluginOps.Append(blit);

    for (int i = 0; i < 5; i++)
    {
        auto blurX = bloomState.frameOpAllocator.Alloc<Frame::FrameCode>();
        blurX->SetName("Bloom Blur X");
        blurX->domain = BarrierDomain::Global;
        blurX->textureDeps.Add(bloomState.internalTargets[0],
                            {
                                "Bloom-Internal0"
                                , CoreGraphics::PipelineStage::ComputeShaderWrite
                                , CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                            });

        blurX->textureDeps.Add(bloomState.blurredBloom,
                            {
                                "Bloom-BlurredBloom"
                                , CoreGraphics::PipelineStage::ComputeShaderRead
                                , CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                            });

        blurX->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
        {
            TextureDimensions dims = TextureGetDimensions(bloomState.internalTargets[0]);

            // calculate execution dimensions
            uint numGroupsX1 = Math::divandroundup(dims.width, BlurBloom::BlurTileWidth);
            uint numGroupsY2 = dims.height;

            CoreGraphics::CmdSetShaderProgram(cmdBuf, bloomState.blurX);
            CoreGraphics::CmdSetResourceTable(cmdBuf, bloomState.blurTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
            CoreGraphics::CmdDispatch(cmdBuf, numGroupsX1, numGroupsY2, 1);
        };

        auto blurY = bloomState.frameOpAllocator.Alloc<Frame::FrameCode>();
        blurY->SetName("Bloom Blur Y");
        blurY->domain = BarrierDomain::Global;
        blurY->textureDeps.Add(bloomState.internalTargets[0],
                              {
                                  "Bloom-Internal0"
                                  , CoreGraphics::PipelineStage::ComputeShaderRead
                                  , CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                              });
        blurY->textureDeps.Add(bloomState.blurredBloom,
                              {
                                  "Bloom-Output"
                                  , CoreGraphics::PipelineStage::ComputeShaderWrite
                                  , CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                              });

        blurY->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
        {
            TextureDimensions dims = TextureGetDimensions(bloomState.internalTargets[0]);

            // calculate execution dimensions
            uint numGroupsX2 = dims.width;
            uint numGroupsY1 = Math::divandroundup(dims.height, BlurBloom::BlurTileWidth);

            CoreGraphics::CmdSetShaderProgram(cmdBuf, bloomState.blurY);
            CoreGraphics::CmdSetResourceTable(cmdBuf, bloomState.blurTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
            CoreGraphics::CmdDispatch(cmdBuf, numGroupsY1, numGroupsX2, 1);
        };

        pluginOps.Append(blurX);
        pluginOps.Append(blurY);
    }

    // Finally add all the operations 
    Frame::AddSubgraph("Bloom", pluginOps);
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

    ResourceTableSetTexture(bloomState.brightPassTable, { bloomState.lightBuffer, Brightpass::Table_Batch::ColorSource_SLOT, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableCommitChanges(bloomState.brightPassTable);

    // bloom buffer goes in, internal target goes out
    ResourceTableSetTexture(bloomState.blurTable, { bloomState.blurredBloom, BlurBloom::Table_Batch::InputImageX_SLOT, 0, CoreGraphics::InvalidSamplerId, false });
    ResourceTableSetRWTexture(bloomState.blurTable, { bloomState.internalTargets[0], BlurBloom::Table_Batch::BlurImageX_SLOT, 0, CoreGraphics::InvalidSamplerId });

    // internal target goes in, blurred buffer goes out
    ResourceTableSetTexture(bloomState.blurTable, { bloomState.internalTargets[0], BlurBloom::Table_Batch::InputImageY_SLOT, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableSetRWTexture(bloomState.blurTable, { bloomState.blurredBloom, BlurBloom::Table_Batch::BlurImageY_SLOT, 0, CoreGraphics::InvalidSamplerId });
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
}

//------------------------------------------------------------------------------
/**
*/
void 
BloomContext::Discard()
{
    bloomState.frameOpAllocator.Release();

    DestroyResourceTable(bloomState.brightPassTable);
    DestroyResourceTable(bloomState.blurTable);
    DestroyTexture(bloomState.internalTargets[0]);
}

} // namespace PostEffects
