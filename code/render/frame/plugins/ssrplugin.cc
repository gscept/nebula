//------------------------------------------------------------------------------
// ssrplugin.cc
// (C) 2019 Individual contributors, see AUTHORS file
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
    this->ssrBufferSlot = ShaderGetResourceSlot(this->shader, "ReflectionBuffer");

    SizeT numFrames = CoreGraphics::GetNumBufferedFrames();

    this->ssrTables.SetSize(numFrames);
    for (IndexT i = 0; i < numFrames; ++i)
    {
        this->ssrTables[i] = ShaderCreateResourceTable(this->shader, NEBULA_BATCH_GROUP);
        ResourceTableSetShaderRWTexture(this->ssrTables[i], { this->readWriteTextures[0], this->ssrBufferSlot, 0, SamplerId::Invalid() });
        ResourceTableCommitChanges(this->ssrTables[i]);
    }

	this->program = ShaderGetProgram(this->shader, ShaderFeatureFromString("Alt0"));
	
    static float alf = 0.0f;
    
	FramePlugin::AddCallback("SSR-Run", [this](IndexT)
	{
        const CameraSettings& cameraSettings = CameraContext::GetSettings(Graphics::GraphicsServer::Instance()->GetCurrentView()->GetCamera());

#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_RED, "Screen Space Reflections");
#endif
        
        const int screenSize[2] = {1680, 1050};

        float sx = (float)screenSize[0] / 2.0f;
        float sy = (float)screenSize[1] / 2.0f;

        const Math::matrix44 scrScale = Math::matrix44({ sx, 0.0f, 0.0f, 0.0f },
                                                       { 0.0f, sy, 0.0f, 0.0f },
                                                       { 0.0f, 0.0f, 1.0f, 0.0f },
                                                       { sx, sy, 0.0f, 1.0f });

        Math::matrix44 viewToTextureSpaceMatrix = Math::matrix44::multiply(cameraSettings.GetProjTransform(), scrScale);

        viewToTextureSpaceMatrix.storeu(ssrBlock.ViewToTextureSpace);
        uint ssrOffset = CoreGraphics::SetComputeConstants(MainThreadConstantBuffer, ssrBlock);

        IndexT frameIndex = CoreGraphics::GetBufferedFrameIndex();

        ResourceTableSetConstantBuffer(this->ssrTables[frameIndex], { this->constants, this->constantsSlot, 0, false, false, sizeof(SsrCs::SSRBlock), (SizeT)ssrOffset });
        ResourceTableCommitChanges(this->ssrTables[frameIndex]);

        CoreGraphics::SetShaderProgram(this->program);
        CoreGraphics::SetResourceTable(this->ssrTables[frameIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);

        const int TILE_SIZE = 32;
        int workGroups[2] = {
            (screenSize[0] + (screenSize[0] % TILE_SIZE)) / TILE_SIZE,
            (screenSize[1] + (screenSize[1] % TILE_SIZE)) / TILE_SIZE
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
	DestroyConstantBuffer(this->constants);
    for (auto& table : this->ssrTables)
    {
	    DestroyResourceTable(table);
        table = CoreGraphics::ResourceTableId::Invalid();
    }
}

} // namespace Algorithms
