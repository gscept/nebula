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

#include "bloom.h"
namespace PostEffects
{

__ImplementPluginContext(PostEffects::BloomContext);
struct
{
    CoreGraphics::ShaderProgramId bloomProgram;
    CoreGraphics::ShaderProgramId downscaleProgram;
    CoreGraphics::ShaderId bloom;

    CoreGraphics::ResourceTableId bloomTable;
    CoreGraphics::BufferId constants;

    CoreGraphics::TextureId bloomBuffer;
    CoreGraphics::TextureId lightBuffer;

    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 1> frameOpAllocator;

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
    bloomState.bloom = ShaderGet("shd:bloom.fxb");
    bloomState.bloomTable = ShaderCreateResourceTable(bloomState.bloom, NEBULA_BATCH_GROUP);

    bloomState.bloomBuffer = script->GetTexture("BloomBuffer");
    bloomState.lightBuffer = script->GetTexture("LightBuffer");
    TextureDimensions dims = TextureGetDimensions(bloomState.bloomBuffer);

    bloomState.bloomProgram = ShaderGetProgram(bloomState.bloom, ShaderFeatureFromString("Bloom"));
    bloomState.downscaleProgram = ShaderGetProgram(bloomState.bloom, ShaderFeatureFromString("Downscale"));

    BufferCreateInfo bufInfo;
    bufInfo.byteSize = Bloom::Table_Batch::BloomUniforms::SIZE;
    bufInfo.usageFlags = ConstantBuffer;
    bufInfo.mode = DeviceAndHost;
    bufInfo.queueSupport = ComputeQueueSupport;
    bloomState.constants = CreateBuffer(bufInfo);

    uint mips = TextureGetNumMips(bloomState.lightBuffer);

    Bloom::BloomUniforms uniforms;
    uniforms.Mips = mips;
    uniforms.Resolution[0] = dims.width;
    uniforms.Resolution[1] = dims.height;
    uniforms.Resolution[2] = 1.0f / dims.width;
    uniforms.Resolution[3] = 1.0f / dims.height;
    BufferUpdate(bloomState.constants, uniforms);

    ResourceTableSetTexture(bloomState.bloomTable, { bloomState.lightBuffer, Bloom::Table_Batch::Input_SLOT });
    ResourceTableSetRWTexture(bloomState.bloomTable, { bloomState.bloomBuffer, Bloom::Table_Batch::Output_SLOT });
    ResourceTableSetConstantBuffer(bloomState.bloomTable, { bloomState.constants, Bloom::Table_Batch::BloomUniforms::SLOT });
    ResourceTableCommitChanges(bloomState.bloomTable);

    Frame::FrameCode* pass = bloomState.frameOpAllocator.Alloc<Frame::FrameCode>();
    pass->SetName("Bloom");
    pass->domain = BarrierDomain::Global;
    pass->textureDeps.Add(
        bloomState.lightBuffer,
        {
            "LightBuffer"
            , PipelineStage::ComputeShaderRead
            , TextureSubresourceInfo::ColorNoLayer(mips)
        });
    pass->textureDeps.Add(
        bloomState.bloomBuffer,
        {
            "BloomBuffer"
            , PipelineStage::ComputeShaderWrite
            , TextureSubresourceInfo::ColorNoMipNoLayer()
        });
    pass->func = [](const CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        TextureDimensions dims = TextureGetDimensions(bloomState.bloomBuffer);
        CmdSetShaderProgram(cmdBuf, bloomState.bloomProgram);
        CmdSetResourceTable(cmdBuf, bloomState.bloomTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = Math::divandroundup(dims.width, 6);
        uint dispatchY = Math::divandroundup(dims.height, 6);
        CmdDispatch(cmdBuf, dispatchX, dispatchY, 1);
    };

    // Finally add all the operations 
    Frame::AddSubgraph("Bloom", { pass });
}

//------------------------------------------------------------------------------
/**
*/
void
BloomContext::WindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
{
    using namespace CoreGraphics;
    TextureRelativeDimensions dims = TextureGetRelativeDimensions(bloomState.bloomBuffer);

    Bloom::BloomUniforms uniforms;
    uniforms.Mips = TextureGetNumMips(bloomState.lightBuffer);
    uniforms.Resolution[0] = dims.width;
    uniforms.Resolution[1] = dims.height;
    BufferUpdate(bloomState.constants, uniforms);

    ResourceTableSetTexture(bloomState.bloomTable, { bloomState.lightBuffer, Bloom::Table_Batch::Input_SLOT });
    ResourceTableSetRWTexture(bloomState.bloomTable, { bloomState.bloomBuffer, Bloom::Table_Batch::Output_SLOT });
    ResourceTableSetConstantBuffer(bloomState.bloomTable, { bloomState.constants, Bloom::Table_Batch::BloomUniforms::SLOT });
    ResourceTableCommitChanges(bloomState.bloomTable);
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

    DestroyResourceTable(bloomState.bloomTable);
}

} // namespace PostEffects
