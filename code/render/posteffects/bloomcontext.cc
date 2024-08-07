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

#include "frame/default.h"
namespace PostEffects
{

__ImplementPluginContext(PostEffects::BloomContext);
struct
{
    CoreGraphics::ShaderProgramId program;
    CoreGraphics::ShaderId shader;

    CoreGraphics::ResourceTableId resourceTable;
    CoreGraphics::BufferId constants;

    Util::FixedArray<CoreGraphics::TextureViewId> lightBufferViews;

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
BloomContext::Setup()
{
    using namespace CoreGraphics;

    // setup shaders
    bloomState.shader = ShaderGet("shd:system_shaders/bloom.fxb");
    bloomState.program = ShaderGetProgram(bloomState.shader, ShaderFeatureMask("Bloom"));
    bloomState.resourceTable = ShaderCreateResourceTable(bloomState.shader, NEBULA_BATCH_GROUP);

    TextureDimensions dims = TextureGetDimensions(FrameScript_default::Texture_BloomBuffer());

    BufferCreateInfo bufInfo;
    bufInfo.byteSize = sizeof(Bloom::BloomUniforms);
    bufInfo.usageFlags = ConstantBuffer;
    bufInfo.mode = DeviceAndHost;
    bufInfo.queueSupport = ComputeQueueSupport;
    bloomState.constants = CreateBuffer(bufInfo);

    uint mips = TextureGetNumMips(FrameScript_default::Texture_LightBuffer());
    bloomState.lightBufferViews.Resize(mips);
    for (IndexT i = 0; i < mips; i++)
    {
        TextureViewCreateInfo inf;
        inf.format = TextureGetPixelFormat(FrameScript_default::Texture_LightBuffer());
        inf.startMip = i;
        inf.numMips = 1;
        inf.tex = FrameScript_default::Texture_LightBuffer();
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

    ResourceTableSetTexture(bloomState.resourceTable, { FrameScript_default::Texture_LightBuffer(), Bloom::Table_Batch::Input_SLOT });
    ResourceTableSetRWTexture(bloomState.resourceTable, { FrameScript_default::Texture_BloomBuffer(), Bloom::Table_Batch::BloomOutput_SLOT });
    ResourceTableSetConstantBuffer(bloomState.resourceTable, { bloomState.constants, Bloom::Table_Batch::BloomUniforms_SLOT });
    ResourceTableCommitChanges(bloomState.resourceTable);


    FrameScript_default::RegisterSubgraph_Bloom_Compute([](const CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, bloomState.program);
        CmdSetResourceTable(cmdBuf, bloomState.resourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
        uint dispatchX = Math::divandroundup(viewport.width(), 6);
        uint dispatchY = Math::divandroundup(viewport.height(), 6);
        CmdDispatch(cmdBuf, dispatchX, dispatchY, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::LightBuffer, PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::BloomBuffer, PipelineStage::ComputeShaderWrite }
    });
}

//------------------------------------------------------------------------------
/**
*/
void
BloomContext::WindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
{
    using namespace CoreGraphics;
    TextureDimensions dims = TextureGetDimensions(FrameScript_default::Texture_BloomBuffer());

    for (auto& view : bloomState.lightBufferViews)
    {
        DestroyTextureView(view);
    }
    bloomState.lightBufferViews.Clear();

    uint mips = TextureGetNumMips(FrameScript_default::Texture_LightBuffer());

    bloomState.lightBufferViews.Resize(mips);
    for (IndexT i = 0; i < mips; i++)
    {
        TextureViewCreateInfo inf;
        inf.format = TextureGetPixelFormat(FrameScript_default::Texture_LightBuffer());
        inf.startMip = i;
        inf.numMips = 1;
        inf.tex = FrameScript_default::Texture_LightBuffer();
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

    ResourceTableSetTexture(bloomState.resourceTable, { FrameScript_default::Texture_LightBuffer(), Bloom::Table_Batch::Input_SLOT });
    ResourceTableSetRWTexture(bloomState.resourceTable, { FrameScript_default::Texture_BloomBuffer(), Bloom::Table_Batch::BloomOutput_SLOT });
    ResourceTableSetConstantBuffer(bloomState.resourceTable, { bloomState.constants, Bloom::Table_Batch::BloomUniforms_SLOT });
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
    DestroyResourceTable(bloomState.resourceTable);
}

} // namespace PostEffects
