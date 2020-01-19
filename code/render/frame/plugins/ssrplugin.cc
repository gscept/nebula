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

    // create shader
	this->shader = ShaderGet("shd:ssr_cs.fxb");
    this->constants = CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer);
    this->constantsSlot = ShaderGetResourceSlot(this->shader, "SSRBlock");
    this->traceBufferSlot = ShaderGetResourceSlot(this->shader, "TraceBuffer");
    
    SizeT numFrames = CoreGraphics::GetNumBufferedFrames();

    this->ssrTables.SetSize(numFrames);
    for (IndexT i = 0; i < numFrames; ++i)
    {
        this->ssrTables[i] = ShaderCreateResourceTable(this->shader, NEBULA_BATCH_GROUP);
        ResourceTableSetRWTexture(this->ssrTables[i], { this->textures["SSRTraceBuffer"], this->traceBufferSlot, 0, SamplerId::Invalid() });
        ResourceTableCommitChanges(this->ssrTables[i]);
    }

	this->program = ShaderGetProgram(this->shader, ShaderFeatureFromString("Alt0"));
	
    FramePlugin::AddCallback("SSR-Trace", [this](IndexT)
	{
        const CameraSettings& cameraSettings = CameraContext::GetSettings(Graphics::GraphicsServer::Instance()->GetCurrentView()->GetCamera());

        Math::matrix44 view = CameraContext::GetTransform(Graphics::GraphicsServer::Instance()->GetCurrentView()->GetCamera());

#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Screen Space Reflections");
#endif
        
        TextureDimensions dims = TextureGetDimensions(this->textures["SSRTraceBuffer"]);

        float sx = (float)dims.width / 2.0f;
        float sy = (float)dims.height / 2.0f;

        const Math::matrix44 scrScale = Math::matrix44({ sx, 0.0f, 0.0f, 0.0f },
                                                       { 0.0f, sy, 0.0f, 0.0f },
                                                       { 0.0f, 0.0f, 1.0f, 0.0f },
                                                       { sx, sy, 0.0f, 1.0f });

        // TODO: for some reason, this doesn't work when passed to the shader. Maybe transpose or something similar?
        Math::matrix44 viewToTextureSpaceMatrix = Math::matrix44::multiply(scrScale, cameraSettings.GetProjTransform());
        
        SsrCs::SSRBlock ssrBlock;
        viewToTextureSpaceMatrix.storeu(ssrBlock.ViewToTextureSpace);
        uint ssrOffset = CoreGraphics::SetComputeConstants(MainThreadConstantBuffer, ssrBlock);

        IndexT frameIndex = CoreGraphics::GetBufferedFrameIndex();

        ResourceTableSetConstantBuffer(this->ssrTables[frameIndex], { this->constants, this->constantsSlot, 0, false, false, sizeof(SsrCs::SSRBlock), (SizeT)ssrOffset });
        ResourceTableCommitChanges(this->ssrTables[frameIndex]);

        CoreGraphics::SetShaderProgram(this->program);
        CoreGraphics::SetResourceTable(this->ssrTables[frameIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);

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
    /*
    FramePlugin::AddCallback("SSR-Resolve", [this](IndexT)
    {
        const CameraSettings& cameraSettings = CameraContext::GetSettings(Graphics::GraphicsServer::Instance()->GetCurrentView()->GetCamera());

        Math::matrix44 view = CameraContext::GetTransform(Graphics::GraphicsServer::Instance()->GetCurrentView()->GetCamera());

#if NEBULA_GRAPHICS_DEBUG
        CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Screen Space Reflections");
#endif

        TextureDimensions dims = TextureGetDimensions(this->textures["ReflectionBuffer"]);

        uint ssrOffset = CoreGraphics::SetGraphicsConstants(MainThreadConstantBuffer, ssrBlock);

        IndexT frameIndex = CoreGraphics::GetBufferedFrameIndex();

        ResourceTableSetConstantBuffer(this->ssrTables[frameIndex], { this->constants, this->constantsSlot, 0, false, false, sizeof(SsrCs::SSRBlock), (SizeT)ssrOffset });
        ResourceTableCommitChanges(this->ssrTables[frameIndex]);

        CoreGraphics::SetShaderProgram(this->program);
        CoreGraphics::SetResourceTable(this->ssrTables[frameIndex], NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);

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
    */
}

//------------------------------------------------------------------------------
/**
*/
void
SSRPlugin::Discard()
{
	FramePlugin::Discard();
    for (auto& table : this->ssrTables)
    {
	    DestroyResourceTable(table);
        table = CoreGraphics::ResourceTableId::Invalid();
    }
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
		ResourceTableSetRWTexture(this->ssrTables[i], { this->textures["SSRTraceBuffer"], this->traceBufferSlot, 0, SamplerId::Invalid() });
		ResourceTableCommitChanges(this->ssrTables[i]);
	}
}

} // namespace Algorithms
