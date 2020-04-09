//------------------------------------------------------------------------------
//  volumetricfogcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "volumetricfogcontext.h"
#include "graphics/graphicsserver.h"
#include "clustering/clustercontext.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"

#include "volumefog.h"
namespace Fog
{

VolumetricFogContext::FogGenericVolumeAllocator VolumetricFogContext::fogGenericVolumeAllocator;
VolumetricFogContext::FogBoxVolumeAllocator VolumetricFogContext::fogBoxVolumeAllocator;
VolumetricFogContext::FogSphereVolumeAllocator VolumetricFogContext::fogSphereVolumeAllocator;
_ImplementContext(VolumetricFogContext, VolumetricFogContext::fogGenericVolumeAllocator);

struct
{
	CoreGraphics::ShaderId classificationShader;
	CoreGraphics::ShaderProgramId cullProgram;
	CoreGraphics::ShaderProgramId renderProgram;

	CoreGraphics::ShaderRWBufferId clusterFogIndexLists;

	Util::FixedArray<CoreGraphics::ShaderRWBufferId> stagingClusterFogLists;
	CoreGraphics::ShaderRWBufferId clusterFogLists;


	CoreGraphics::TextureId fogVolumeTexture0;
	CoreGraphics::TextureId fogVolumeTexture1;

	IndexT clusterUniformsSlot;
	IndexT uniformsSlot;
	IndexT lightingTextureSlot;
	IndexT clusterUniforms;

	Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;
	float turbidity;
	Math::float4 color;

	// these are used to update the light clustering
	Volumefog::FogBox fogBoxes[128];
	Volumefog::FogSphere fogSpheres[128];
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

	IndexT fogIndexListsSlot = ShaderGetResourceSlot(fogState.classificationShader, "FogIndexLists");
	IndexT fogListsSlot = ShaderGetResourceSlot(fogState.classificationShader, "FogLists");

	IndexT lightIndexListsSlot = ShaderGetResourceSlot(fogState.classificationShader, "LightIndexLists");
	IndexT lightListsSlot = ShaderGetResourceSlot(fogState.classificationShader, "LightLists");
	IndexT clusterAABBSlot = ShaderGetResourceSlot(fogState.classificationShader, "ClusterAABBs");

	ShaderRWBufferCreateInfo rwbInfo =
	{
		"FogIndexListsBuffer",
		sizeof(Volumefog::FogIndexLists),
		BufferUpdateMode::DeviceWriteable,
		false
	};
	fogState.clusterFogIndexLists = CreateShaderRWBuffer(rwbInfo);

	rwbInfo.name = "FogLists";
	rwbInfo.size = sizeof(Volumefog::FogLists);
	fogState.clusterFogLists = CreateShaderRWBuffer(rwbInfo);

	rwbInfo.mode = BufferUpdateMode::HostWriteable;
	fogState.stagingClusterFogLists.Resize(CoreGraphics::GetNumBufferedFrames());

	fogState.cullProgram = ShaderGetProgram(fogState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Cull"));
	fogState.renderProgram = ShaderGetProgram(fogState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Render"));

	fogState.resourceTables.Resize(CoreGraphics::GetNumBufferedFrames());
	for (IndexT i = 0; i < fogState.resourceTables.Size(); i++)
	{
		fogState.resourceTables[i] = ShaderCreateResourceTable(fogState.classificationShader, NEBULA_BATCH_GROUP);
		fogState.stagingClusterFogLists[i] = CreateShaderRWBuffer(rwbInfo);

		ResourceTableSetRWBuffer(fogState.resourceTables[i], { Clustering::ClusterContext::GetClusterBuffer(), clusterAABBSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetRWBuffer(fogState.resourceTables[i], { Lighting::LightContext::GetLightIndexBuffer(), lightIndexListsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetRWBuffer(fogState.resourceTables[i], { Lighting::LightContext::GetLightsBuffer(), lightListsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetRWBuffer(fogState.resourceTables[i], { fogState.clusterFogIndexLists, fogIndexListsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetRWBuffer(fogState.resourceTables[i], { fogState.clusterFogLists, fogListsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
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

	fogState.turbidity = 0.05f;
	fogState.color = Math::float4(1);

	_CreateContext();
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
VolumetricFogContext::SetupBoxVolume(
	const Graphics::GraphicsEntityId id, 
	const Math::matrix44& transform, 
	const float density, 
	const Math::float4 absorption)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	Ids::Id32 fog = fogBoxVolumeAllocator.Alloc();
	fogBoxVolumeAllocator.Set<FogBoxVolume_Transform>(fog, transform);

	fogGenericVolumeAllocator.Set<FogVolume_Type>(cid.id, BoxVolume);
	fogGenericVolumeAllocator.Set<FogVolume_TypedId>(cid.id, fog);
	fogGenericVolumeAllocator.Set<FogVolume_Density>(cid.id, density);
	fogGenericVolumeAllocator.Set<FogVolume_Absorption>(cid.id, absorption);
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::SetupSphereVolume(
	const Graphics::GraphicsEntityId id, 
	Math::float4 position, 
	float radius, 
	const float density, 
	const Math::float4 absorption)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	Ids::Id32 fog = fogSphereVolumeAllocator.Alloc();
	fogSphereVolumeAllocator.Set<FogSphereVolume_Position>(fog, position);
	fogSphereVolumeAllocator.Set<FogSphereVolume_Radius>(fog, radius);

	fogGenericVolumeAllocator.Set<FogVolume_Type>(cid.id, SphereVolume);
	fogGenericVolumeAllocator.Set<FogVolume_TypedId>(cid.id, fog);
	fogGenericVolumeAllocator.Set<FogVolume_Density>(cid.id, density);
	fogGenericVolumeAllocator.Set<FogVolume_Absorption>(cid.id, absorption);
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
	using namespace CoreGraphics;
	IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();
	Math::matrix44 viewTransform = Graphics::CameraContext::GetTransform(view->GetCamera());

	SizeT numFogBoxVolumes = 0;
	SizeT numFogSphereVolumes = 0;

	const Util::Array<FogVolumeType>& types = fogGenericVolumeAllocator.GetArray<FogVolume_Type>();
	const Util::Array<Ids::Id32>& typeIds = fogGenericVolumeAllocator.GetArray<FogVolume_TypedId>();
	IndexT i;
	for (i = 0; i < types.Size(); i++)
	{
		switch (types[i])
		{
		case BoxVolume:
		{
			auto& fog = fogState.fogBoxes[numFogBoxVolumes];
			Math::float4::storeu3(fogGenericVolumeAllocator.Get<FogVolume_Absorption>(i), fog.absorption);
			fog.turbidity = fogGenericVolumeAllocator.Get<FogVolume_Density>(i);
			fog.falloff = 64.0f;
			Math::matrix44 transform = Math::matrix44::multiply(fogBoxVolumeAllocator.Get<FogBoxVolume_Transform>(typeIds[i]), viewTransform);
			Math::bbox box(transform);
			Math::float4::storeu3(box.pmin, fog.bboxMin);
			Math::float4::storeu3(box.pmax, fog.bboxMax);
			Math::matrix44::storeu(Math::matrix44::inverse(transform), fog.invTransform);
			numFogBoxVolumes++;
			break;
		}
		case SphereVolume:
		{
			auto& fog = fogState.fogSpheres[numFogSphereVolumes];
			Math::float4::storeu3(fogGenericVolumeAllocator.Get<FogVolume_Absorption>(i), fog.absorption);
			fog.turbidity = fogGenericVolumeAllocator.Get<FogVolume_Density>(i);
			Math::float4 pos = fogSphereVolumeAllocator.Get<FogSphereVolume_Position>(typeIds[i]);
			pos = Math::matrix44::transform(pos, viewTransform);
			Math::float4::storeu3(pos, fog.position);
			fog.radius = fogSphereVolumeAllocator.Get<FogSphereVolume_Radius>(typeIds[i]);
			fog.falloff = 64.0f;
			numFogSphereVolumes++;
			break;
		}
		}
	}

	// update list of point lights
	if (numFogBoxVolumes > 0 || numFogSphereVolumes > 0)
	{
		Volumefog::FogLists fogList;
		Memory::CopyElements(fogState.fogBoxes, fogList.FogBoxes, numFogBoxVolumes);
		Memory::CopyElements(fogState.fogSpheres, fogList.FogSpheres, numFogSphereVolumes);
		CoreGraphics::ShaderRWBufferUpdate(fogState.stagingClusterFogLists[bufferIndex], &fogList, sizeof(Volumefog::FogLists));
	}

	Volumefog::VolumeFogUniforms fogUniforms;
	fogUniforms.NumFogBoxes = numFogBoxVolumes;
	fogUniforms.NumFogSpheres = numFogSphereVolumes;
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
	// update constants
	using namespace CoreGraphics;

	const IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

	// copy data from staging buffer to shader buffer
	BarrierInsert(ComputeQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::Transfer,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				fogState.clusterFogLists,
				BarrierAccess::ShaderRead,
				BarrierAccess::TransferWrite,
				0, NEBULA_WHOLE_BUFFER_SIZE
			},
		}, "Decals data upload");
	Copy(ComputeQueueType, fogState.stagingClusterFogLists[bufferIndex], 0, fogState.clusterFogLists, 0, sizeof(Volumefog::FogLists));
	BarrierInsert(ComputeQueueType,
		BarrierStage::Transfer,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				fogState.clusterFogLists,
				BarrierAccess::TransferWrite,
				BarrierAccess::ShaderRead,
				0, NEBULA_WHOLE_BUFFER_SIZE
			},
		}, "Decals data upload");

	// begin command buffer work
	CommandBufferBeginMarker(ComputeQueueType, NEBULA_MARKER_BLUE, "Volumefog cluster culling");

	// make sure to sync so we don't read from data that is being written...
	BarrierInsert(ComputeQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				fogState.clusterFogIndexLists,
				BarrierAccess::ShaderRead,
				BarrierAccess::ShaderWrite,
				0, NEBULA_WHOLE_BUFFER_SIZE
			},
		}, "Decals cluster culling begin");

	SetShaderProgram(fogState.cullProgram, ComputeQueueType);
	SetResourceTable(fogState.resourceTables[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr, ComputeQueueType);

	// run chunks of 1024 threads at a time
	std::array<SizeT, 3> dimensions = Clustering::ClusterContext::GetClusterDimensions();
	Compute(Math::n_ceil((dimensions[0] * dimensions[1] * dimensions[2]) / 64.0f), 1, 1, ComputeQueueType);

	// make sure to sync so we don't read from data that is being written...
	BarrierInsert(ComputeQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				fogState.clusterFogIndexLists,
				BarrierAccess::ShaderWrite,
				BarrierAccess::ShaderRead,
				0, NEBULA_WHOLE_BUFFER_SIZE
			},
		}, "Decals cluster culling end");

	CommandBufferEndMarker(ComputeQueueType);
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

	// no need for an explicit barrier here, because the framescript will assume fog0 is write/general and will sync automatically

	CommandBufferEndMarker(GraphicsQueueType);
}

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId 
VolumetricFogContext::Alloc()
{
	return fogGenericVolumeAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::Dealloc(Graphics::ContextEntityId id)
{
	fogGenericVolumeAllocator.Dealloc(id.id);
}

} // namespace Fog
