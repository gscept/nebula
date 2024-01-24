//------------------------------------------------------------------------------
//  bloomcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "frame/framesubgraph.h"
#include "frame/framecode.h"
#include "coregraphics/resourcetable.h"
#include "graphics/graphicsserver.h"
#include "bloomcontext.h"

#include "system_shaders/bloom.h"
namespace PostEffects
{

__ImplementPluginContext(PostEffects::BloomContext);
struct
{
    CoreGraphics::ShaderProgramId program;
    CoreGraphics::ShaderId shader;

    CoreGraphics::ResourceTableId resourceTable;
    CoreGraphics::BufferId constants;

    CoreGraphics::TextureId bloomBuffer;
    CoreGraphics::TextureId lightBuffer;

    Util::FixedArray<CoreGraphics::TextureViewId> lightBufferViews;

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
    bloomState.shader = ShaderGet("shd:bloom.fxb");
    bloomState.program = ShaderGetProgram(bloomState.shader, ShaderFeatureFromString("Bloom"));
    bloomState.resourceTable = ShaderCreateResourceTable(bloomState.shader, NEBULA_BATCH_GROUP);

    bloomState.bloomBuffer = script->GetTexture("BloomBuffer");
    bloomState.lightBuffer = script->GetTexture("LightBuffer");
    TextureDimensions dims = TextureGetDimensions(bloomState.bloomBuffer);

    BufferCreateInfo bufInfo;
    bufInfo.byteSize = Bloom::Table_Batch::BloomUniforms::SIZE;
    bufInfo.usageFlags = ConstantBuffer;
    bufInfo.mode = DeviceAndHost;
    bufInfo.queueSupport = ComputeQueueSupport;
    bloomState.constants = CreateBuffer(bufInfo);

    uint mips = TextureGetNumMips(bloomState.lightBuffer);
    bloomState.lightBufferViews.Resize(mips);
    for (IndexT i = 0; i < mips; i++)
    {
        TextureViewCreateInfo inf;
        inf.format = TextureGetPixelFormat(bloomState.lightBuffer);
        inf.startMip = i;
        inf.numMips = 1;
        inf.tex = bloomState.lightBuffer;
        bloomState.lightBufferViews[i] = CreateTextureView(inf);
    }

    Bloom::BloomUniforms uniforms;
    uniforms.Mips = mips;
    for (IndexT i = 0; i < mips; i++)
    {
        uniforms.Resolutions[i][0] = dims.width >> i;
        uniforms.Resolutions[i][1] = dims.height >> i;
        uniforms.Resolutions[i][2] = 1.0f / (dims.width >> i);
        uniforms.Resolutions[i][3] = 1.0f / (dims.height >> i);
    }
    BufferUpdate(bloomState.constants, uniforms);

    ResourceTableSetTexture(bloomState.resourceTable, { bloomState.lightBuffer, Bloom::Table_Batch::Input_SLOT });
    ResourceTableSetRWTexture(bloomState.resourceTable, { bloomState.bloomBuffer, Bloom::Table_Batch::BloomOutput_SLOT });
    ResourceTableSetConstantBuffer(bloomState.resourceTable, { bloomState.constants, Bloom::Table_Batch::BloomUniforms::SLOT });
    ResourceTableCommitChanges(bloomState.resourceTable);

    Frame::FrameCode* upscale = bloomState.frameOpAllocator.Alloc<Frame::FrameCode>();
    upscale->SetName("Bloom");
    upscale->domain = BarrierDomain::Global;
    upscale->textureDeps.Add(
        bloomState.lightBuffer,
        {
            "LightBuffer"
            , PipelineStage::ComputeShaderRead
            , TextureSubresourceInfo::Color(bloomState.lightBuffer)
        });
    upscale->textureDeps.Add(
        bloomState.bloomBuffer,
        {
            "BloomBuffer"
            , PipelineStage::ComputeShaderWrite
            , TextureSubresourceInfo::Color(bloomState.bloomBuffer)
        });
    upscale->func = [](const CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        TextureDimensions dims = TextureGetDimensions(bloomState.bloomBuffer);
        CmdSetShaderProgram(cmdBuf, bloomState.program);
        CmdSetResourceTable(cmdBuf, bloomState.resourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = Math::divandroundup(dims.width, 6);
        uint dispatchY = Math::divandroundup(dims.height, 6);
        CmdDispatch(cmdBuf, dispatchX, dispatchY, 1);
    };
    
    Frame::AddSubgraph("Bloom", { upscale });
}

//------------------------------------------------------------------------------
/**
*/
void
BloomContext::WindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
{
    using namespace CoreGraphics;
    TextureDimensions dims = TextureGetDimensions(bloomState.bloomBuffer);

    for (auto& view : bloomState.lightBufferViews)
    {
        DestroyTextureView(view);
    }
    bloomState.lightBufferViews.Clear();

    uint mips = TextureGetNumMips(bloomState.lightBuffer);

    bloomState.lightBufferViews.Resize(mips);
    for (IndexT i = 0; i < mips; i++)
    {
        TextureViewCreateInfo inf;
        inf.format = TextureGetPixelFormat(bloomState.lightBuffer);
        inf.startMip = i;
        inf.numMips = 1;
        inf.tex = bloomState.lightBuffer;
        bloomState.lightBufferViews[i] = CreateTextureView(inf);
    }

    Bloom::BloomUniforms uniforms;
    uniforms.Mips = mips;
    for (IndexT i = 0; i < mips; i++)
    {
        uniforms.Resolutions[i][0] = dims.width >> i;
        uniforms.Resolutions[i][1] = dims.height >> i;
        uniforms.Resolutions[i][2] = 1.0f / (dims.width >> i);
        uniforms.Resolutions[i][3] = 1.0f / (dims.height >> i);
    }
    BufferUpdate(bloomState.constants, uniforms);

    ResourceTableSetTexture(bloomState.resourceTable, { bloomState.lightBuffer, Bloom::Table_Batch::Input_SLOT });
    ResourceTableSetRWTexture(bloomState.resourceTable, { bloomState.bloomBuffer, Bloom::Table_Batch::BloomOutput_SLOT });
    ResourceTableSetConstantBuffer(bloomState.resourceTable, { bloomState.constants, Bloom::Table_Batch::BloomUniforms::SLOT });
    ResourceTableCommitChanges(bloomState.resourceTable);
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

    DestroyResourceTable(bloomState.resourceTable);
}

} // namespace PostEffects
