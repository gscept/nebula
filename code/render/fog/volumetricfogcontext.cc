//------------------------------------------------------------------------------
//  volumetricfogcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "volumetricfogcontext.h"
#include "clustering/clustercontext.h"
#include "graphics/cameracontext.h"

#include "volumefog.h"
namespace Fog
{

VolumetricFogContext::FogVolumeAllocator VolumetricFogContext::fogVolumeAllocator;
_ImplementContext(VolumetricFogContext, VolumetricFogContext::fogVolumeAllocator);

struct
{
	CoreGraphics::ShaderId classificationShader;
	CoreGraphics::ShaderProgramId cullProgram;
	CoreGraphics::ShaderProgramId renderProgram;

	CoreGraphics::TextureId fogVolumeTexture0;
	CoreGraphics::TextureId fogVolumeTexture1;

	IndexT clusterUniformsSlot;
	IndexT uniformsSlot;
	IndexT lightingTextureSlot;
	IndexT clusterUniforms;

	Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;
	float turbidity;
	Math::float4 color;
} fogState;

struct
{
	CoreGraphics::ShaderId blurShader;
	CoreGraphics::ShaderProgramId blurXProgram, blurYProgram;
	Util::FixedArray<CoreGraphics::ResourceTableId> blurXTable, blurYTable;
	IndexT blurInputXSlot, blurInputYSlot, blurOutputXSlot, blurOutputYSlot;
} blurState;


//------------------------------------------------------------------------------
/**
*/
VolumetricFogContext::VolumetricFogContext()
{

}

//------------------------------------------------------------------------------
/**
*/
VolumetricFogContext::~VolumetricFogContext()
{

}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::Create()
{
	__bundle.OnUpdateViewResources = VolumetricFogContext::UpdateViewDependentResources;

	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);


	Frame::FramePlugin::AddCallback("VolumetricFogContext - Cull and Classify", [](IndexT frame)
		{
			VolumetricFogContext::CullAndClassify();
		});

	Frame::FramePlugin::AddCallback("VolumetricFogContext - Render", [](IndexT frame)
		{
			VolumetricFogContext::Render();
		});

	using namespace CoreGraphics;
	fogState.classificationShader = ShaderServer::Instance()->GetShader("shd:volumefog.fxb");
	fogState.clusterUniformsSlot = ShaderGetResourceSlot(fogState.classificationShader, "ClusterUniforms");
	fogState.uniformsSlot = ShaderGetResourceSlot(fogState.classificationShader, "VolumeFogUniforms");
	fogState.lightingTextureSlot = ShaderGetResourceSlot(fogState.classificationShader, "Lighting");
	IndexT lightIndexListsSlot = ShaderGetResourceSlot(fogState.classificationShader, "LightIndexLists");
	IndexT lightListsSlot = ShaderGetResourceSlot(fogState.classificationShader, "LightLists");
	IndexT clusterAABBSlot = ShaderGetResourceSlot(fogState.classificationShader, "ClusterAABBs");

	fogState.cullProgram = ShaderGetProgram(fogState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Cull"));
	fogState.renderProgram = ShaderGetProgram(fogState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Render"));

	fogState.resourceTables.Resize(CoreGraphics::GetNumBufferedFrames());
	for (IndexT i = 0; i < fogState.resourceTables.Size(); i++)
	{
		fogState.resourceTables[i] = ShaderCreateResourceTable(fogState.classificationShader, NEBULA_BATCH_GROUP);

		ResourceTableSetRWBuffer(fogState.resourceTables[i], { Clustering::ClusterContext::GetClusterBuffer(), clusterAABBSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetRWBuffer(fogState.resourceTables[i], { Lighting::LightContext::GetLightIndexBuffer(), lightIndexListsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetRWBuffer(fogState.resourceTables[i], { Lighting::LightContext::GetLightsBuffer(), lightListsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetConstantBuffer(fogState.resourceTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), fogState.clusterUniformsSlot, 0, false, false, sizeof(Volumefog::ClusterUniforms), 0 });
		ResourceTableSetConstantBuffer(fogState.resourceTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), fogState.uniformsSlot, 0, false, false, sizeof(Volumefog::VolumeFogUniforms), 0 });
	}

	blurState.blurShader = ShaderServer::Instance()->GetShader("shd:blur_2d_rgba16f_cs.fxb");
	blurState.blurXProgram = ShaderGetProgram(blurState.blurShader, ShaderServer::Instance()->FeatureStringToMask("Alt0"));
	blurState.blurYProgram = ShaderGetProgram(blurState.blurShader, ShaderServer::Instance()->FeatureStringToMask("Alt1"));
	blurState.blurXTable.Resize(CoreGraphics::GetNumBufferedFrames());
	blurState.blurYTable.Resize(CoreGraphics::GetNumBufferedFrames()); 

	for (IndexT i = 0; i < blurState.blurXTable.Size(); i++)
	{
		blurState.blurXTable[i] = ShaderCreateResourceTable(blurState.blurShader, NEBULA_BATCH_GROUP);
		blurState.blurYTable[i] = ShaderCreateResourceTable(blurState.blurShader, NEBULA_BATCH_GROUP);
	}

	blurState.blurInputXSlot = ShaderGetResourceSlot(blurState.blurShader, "InputImageX");
	blurState.blurInputYSlot = ShaderGetResourceSlot(blurState.blurShader, "InputImageY");
	blurState.blurOutputXSlot = ShaderGetResourceSlot(blurState.blurShader, "BlurImageX");
	blurState.blurOutputYSlot = ShaderGetResourceSlot(blurState.blurShader, "BlurImageY");

	fogState.turbidity = 0.2f;
	fogState.color = Math::float4(1);
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::Discard()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::SetupVolume(
	const Graphics::GraphicsEntityId id, 
	const Math::matrix44& transform, 
	const float density, 
	const Math::float4 absorption)
{
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
	using namespace CoreGraphics;
	IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();
	Volumefog::VolumeFogUniforms fogUniforms;
	fogUniforms.GlobalTurbidity = fogState.turbidity;
	Math::float4::storeu3(fogState.color, fogUniforms.GlobalAbsorption);
	fogUniforms.Downscale = 4;

	CoreGraphics::TextureId fog0 = view->GetFrameScript()->GetTexture("VolumetricFogBuffer0");
	CoreGraphics::TextureId fog1 = view->GetFrameScript()->GetTexture("VolumetricFogBuffer1");

	fogState.fogVolumeTexture0 = fog0;
	fogState.fogVolumeTexture1 = fog1;
	TextureDimensions dims = TextureGetDimensions(fog0);

	uint offset = SetComputeConstants(MainThreadConstantBuffer, fogUniforms);
	ResourceTableSetConstantBuffer(fogState.resourceTables[bufferIndex], { GetComputeConstantBuffer(MainThreadConstantBuffer), fogState.uniformsSlot, 0, false, false, sizeof(Volumefog::VolumeFogUniforms), (SizeT)offset });

	ClusterGenerate::ClusterUniforms clusterUniforms = Clustering::ClusterContext::GetUniforms();
	clusterUniforms.FramebufferDimensions[0] = dims.width;
	clusterUniforms.FramebufferDimensions[1] = dims.height;
	clusterUniforms.InvFramebufferDimensions[0] = 1 / float(dims.width);
	clusterUniforms.InvFramebufferDimensions[1] = 1 / float(dims.height);
	offset = SetComputeConstants(MainThreadConstantBuffer, clusterUniforms);
	ResourceTableSetConstantBuffer(fogState.resourceTables[bufferIndex], { GetComputeConstantBuffer(MainThreadConstantBuffer), fogState.clusterUniformsSlot, 0, false, false, sizeof(ClusterGenerate::ClusterUniforms), (SizeT)offset });
	
	ResourceTableSetRWTexture(fogState.resourceTables[bufferIndex], { fog0, fogState.lightingTextureSlot, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableCommitChanges(fogState.resourceTables[bufferIndex]);


	// setup blur tables
	ResourceTableSetTexture(blurState.blurXTable[bufferIndex], { fog0, blurState.blurInputXSlot, 0, CoreGraphics::SamplerId::Invalid(), false }); // ping
	ResourceTableSetRWTexture(blurState.blurXTable[bufferIndex], { fog1, blurState.blurOutputXSlot, 0, CoreGraphics::SamplerId::Invalid() }); // pong
	ResourceTableSetTexture(blurState.blurYTable[bufferIndex], { fog1, blurState.blurInputYSlot, 0, CoreGraphics::SamplerId::Invalid() }); // ping
	ResourceTableSetRWTexture(blurState.blurYTable[bufferIndex], { fog0, blurState.blurOutputYSlot, 0, CoreGraphics::SamplerId::Invalid() }); // pong
	ResourceTableCommitChanges(blurState.blurXTable[bufferIndex]);
	ResourceTableCommitChanges(blurState.blurYTable[bufferIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::SetGlobalTurbidity(float f)
{
	fogState.turbidity = f;
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::SetGlobalAbsorption(const Math::float4& color)
{
	fogState.color = color;
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::CullAndClassify()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::Render()
{
	using namespace CoreGraphics;
	TextureDimensions dims = TextureGetDimensions(fogState.fogVolumeTexture0);
	const IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

	CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Volumetric Fog");

	SetShaderProgram(fogState.renderProgram, GraphicsQueueType);
	SetResourceTable(fogState.resourceTables[bufferIndex], NEBULA_BATCH_GROUP, ComputePipeline, nullptr, GraphicsQueueType);

	// run volumetric fog compute
	Compute(Math::n_divandroundup(dims.width, 64), dims.height, 1, GraphicsQueueType);

	CommandBufferEndMarker(GraphicsQueueType);

	// now blur
	CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Volumetric Blur");

	// insert barrier after compute and before blur
	BarrierInsert(GraphicsQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		{
			TextureBarrier
			{
				fogState.fogVolumeTexture0,
				ImageSubresourceInfo::ColorNoMipNoLayer(),
				ImageLayout::General,
				ImageLayout::ShaderRead,
				BarrierAccess::ShaderWrite,
				BarrierAccess::ShaderRead,
			},
			TextureBarrier
			{
				fogState.fogVolumeTexture1,
				ImageSubresourceInfo::ColorNoMipNoLayer(),
				ImageLayout::ShaderRead,
				ImageLayout::General,
				BarrierAccess::ShaderRead,
				BarrierAccess::ShaderWrite,
			},
		},
		nullptr, "Fog volume update finish barrier");



	SetShaderProgram(blurState.blurXProgram, GraphicsQueueType);
	SetResourceTable(blurState.blurXTable[bufferIndex], NEBULA_BATCH_GROUP, ComputePipeline, nullptr, GraphicsQueueType);

	// fog0 -> read, fog1 -> write
#define TILE_WIDTH 320
	Compute(Math::n_divandroundup(dims.width, TILE_WIDTH), dims.height, 1, GraphicsQueueType);

	BarrierInsert(GraphicsQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		{
			TextureBarrier
			{
				fogState.fogVolumeTexture0,
				ImageSubresourceInfo::ColorNoMipNoLayer(),
				ImageLayout::ShaderRead,
				ImageLayout::General,
				BarrierAccess::ShaderRead,
				BarrierAccess::ShaderWrite,
			},
			TextureBarrier
			{
				fogState.fogVolumeTexture1,
				ImageSubresourceInfo::ColorNoMipNoLayer(),
				ImageLayout::General,
				ImageLayout::ShaderRead,
				BarrierAccess::ShaderWrite,
				BarrierAccess::ShaderRead,
			},
		},
		nullptr, "Fog volume update finish barrier");

	SetShaderProgram(blurState.blurYProgram, GraphicsQueueType);
	SetResourceTable(blurState.blurYTable[bufferIndex], NEBULA_BATCH_GROUP, ComputePipeline, nullptr, GraphicsQueueType);

	// fog0 -> write, fog1 -> read
	Compute(Math::n_divandroundup(dims.height, TILE_WIDTH), dims.width, 1, GraphicsQueueType);

	// no need for an explicit barrier here, because the framescript will assume fog0 is write/general

	CommandBufferEndMarker(GraphicsQueueType);
}

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId 
VolumetricFogContext::Alloc()
{
	return fogVolumeAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::Dealloc(Graphics::ContextEntityId id)
{
	fogVolumeAllocator.Dealloc(id.id);
}

} // namespace Fog
