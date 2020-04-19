//------------------------------------------------------------------------------
// ssrplugin.cc
// (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "ssrplugin.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/graphicsdevice.h"
#include "framesync/framesynctimer.h"
#include "core/sysfunc.h"
#include "graphics/graphicsserver.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"
#include "ssr_cs.h"

using namespace CoreGraphics;
using namespace Graphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
SSRPlugin::SSRPlugin()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
SSRPlugin::~SSRPlugin()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
SSRPlugin::Setup()
{
	FramePlugin::Setup();
    
    SizeT numFrames = CoreGraphics::GetNumBufferedFrames();

    // create trace shader
	this->traceShader = ShaderGet("shd:ssr_cs.fxb");
    this->constants = CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer);
    this->constantsSlot = ShaderGetResourceSlot(this->traceShader, "SSRBlock");
    this->traceBufferSlot = ShaderGetResourceSlot(this->traceShader, "TraceBuffer");

    this->ssrTraceTables.SetSize(numFrames);
    for (IndexT i = 0; i < numFrames; ++i)
    {
        this->ssrTraceTables[i] = ShaderCreateResourceTable(this->traceShader, NEBULA_BATCH_GROUP);
        ResourceTableSetRWTexture(this->ssrTraceTables[i], { this->textures["SSRTraceBuffer"], this->traceBufferSlot, 0, SamplerId::Invalid() });
        ResourceTableCommitChanges(this->ssrTraceTables[i]);
    }

    //create resolve shader
    this->resolveShader = ShaderGet("shd:ssr_resolve_cs.fxb");
    this->reflectionBufferSlot = ShaderGetResourceSlot(this->resolveShader, "ReflectionBuffer");
    this->lightBufferSlot = ShaderGetResourceSlot(this->resolveShader, "LightBuffer");
    this->resolveTraceBufferSlot = ShaderGetResourceSlot(this->resolveShader, "TraceBuffer");

    this->ssrResolveTables.SetSize(numFrames);
    for (IndexT i = 0; i < numFrames; ++i)
    {
        this->ssrResolveTables[i] = ShaderCreateResourceTable(this->resolveShader, NEBULA_BATCH_GROUP);
        ResourceTableSetRWTexture(this->ssrResolveTables[i], { this->textures["ReflectionBuffer"], this->reflectionBufferSlot, 0, SamplerId::Invalid() });
        ResourceTableSetTexture(this->ssrResolveTables[i], { this->textures["SSRTraceBuffer"], this->resolveTraceBufferSlot, 0, SamplerId::Invalid() });
        ResourceTableCommitChanges(this->ssrResolveTables[i]);
    }

    // setup programs
	this->traceProgram = ShaderGetProgram(this->traceShader, ShaderFeatureFromString("Alt0"));
	this->resolveProgram = ShaderGetProgram(this->resolveShader, ShaderFeatureFromString("Alt0"));
	
    FramePlugin::AddCallback("SSR-Trace", [this](IndexT)
	{
#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Screen Space Reflections");
#endif
        TextureDimensions dims = TextureGetDimensions(this->textures["SSRTraceBuffer"]);
        IndexT frameIndex = CoreGraphics::GetBufferedFrameIndex();

        CoreGraphics::SetShaderProgram(this->traceProgram);
        CoreGraphics::SetResourceTable(this->ssrTraceTables[frameIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);

        const int TILE_SIZE = 32;
        int workGroups[2] = {
            (dims.width + (dims.width % TILE_SIZE)) / TILE_SIZE,
            (dims.height + (dims.height % TILE_SIZE)) / TILE_SIZE
        };
        CoreGraphics::Compute(workGroups[0], workGroups[1], 1);

#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
	});

    FramePlugin::AddCallback("SSR-Resolve", [this](IndexT)
	{
#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Screen Space Reflections");
#endif
        TextureDimensions dims = TextureGetDimensions(this->textures["ReflectionBuffer"]);
        IndexT frameIndex = CoreGraphics::GetBufferedFrameIndex();

        CoreGraphics::SetShaderProgram(this->resolveProgram);
        CoreGraphics::SetResourceTable(this->ssrResolveTables[frameIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);

        const int TILE_SIZE = 32;
        int workGroups[2] = {
            (dims.width + (dims.width % TILE_SIZE)) / TILE_SIZE,
            (dims.height + (dims.height % TILE_SIZE)) / TILE_SIZE
        };
        CoreGraphics::Compute(workGroups[0], workGroups[1], 1);

#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
	});
}

//------------------------------------------------------------------------------
/**
*/
void
SSRPlugin::Discard()
{
	FramePlugin::Discard();
    for (auto& table : this->ssrTraceTables)
    {
	    DestroyResourceTable(table);
        table = CoreGraphics::ResourceTableId::Invalid();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SSRPlugin::UpdateViewDependentResources(const Ptr<Graphics::View>& view, const IndexT frameIndex)
{
    const CameraSettings& cameraSettings = CameraContext::GetSettings(view->GetCamera());
    TextureDimensions dims = TextureGetDimensions(this->textures["SSRTraceBuffer"]);

    float sx = (float)dims.width;
    float sy = (float)dims.height;

    const Math::mat4 scrScale = Math::mat4(
        Math::vec4(sx, 0.0f, 0.0f, 0.0f),
        Math::vec4(0.0f, sy, 0.0f, 0.0f),
        Math::vec4(0.0f, 0.0f, 1.0f, 0.0f),
        Math::vec4(sx, sy, 0.0f, 1.0f)
    );

    Math::mat4 conv = cameraSettings.GetProjTransform();
    conv.r[1] = -conv.r[1];

    Math::mat4 viewToTextureSpaceMatrix = conv * scrScale;

    SsrCs::SSRBlock ssrBlock;
    viewToTextureSpaceMatrix.store(ssrBlock.ViewToTextureSpace);
    uint ssrOffset = CoreGraphics::SetComputeConstants(MainThreadConstantBuffer, ssrBlock);

    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    ResourceTableSetConstantBuffer(this->ssrTraceTables[bufferIndex], { this->constants, this->constantsSlot, 0, false, false, sizeof(SsrCs::SSRBlock), (SizeT)ssrOffset });
    ResourceTableCommitChanges(this->ssrTraceTables[bufferIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void
SSRPlugin::Resize()
{
	SizeT numFrames = CoreGraphics::GetNumBufferedFrames();
	for (IndexT i = 0; i < numFrames; ++i)
	{
		ResourceTableSetRWTexture(this->ssrTraceTables[i], { this->textures["SSRTraceBuffer"], this->traceBufferSlot, 0, SamplerId::Invalid() });
		ResourceTableCommitChanges(this->ssrTraceTables[i]);
	}
}

} // namespace Algorithms
