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
#include "lighting/lightcontext.h"
#include "frame/frameplugin.h"
#include "imgui.h"

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

	CoreGraphics::BufferId clusterFogIndexLists;

	Util::FixedArray<CoreGraphics::BufferId> stagingClusterFogLists;
	CoreGraphics::BufferId clusterFogLists;

	CoreGraphics::TextureId fogVolumeTexture0;
	CoreGraphics::TextureId fogVolumeTexture1;

	IndexT uniformsSlot;
	IndexT lightingTextureSlot;
	IndexT clusterUniforms;

	Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;
	float turbidity;
	Math::vec3 color;

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
	__bundle.OnBegin = VolumetricFogContext::RenderUI;

	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	Frame::AddCallback("VolumetricFogContext - Cull and Classify", [](const IndexT frame, const IndexT bufferIndex)
		{
			VolumetricFogContext::CullAndClassify();
		});

	Frame::AddCallback("VolumetricFogContext - Render", [](const IndexT frame, const IndexT bufferIndex)
		{
			VolumetricFogContext::Render();
		});

	using namespace CoreGraphics;
	fogState.classificationShader = ShaderServer::Instance()->GetShader("shd:volumefog.fxb");
	fogState.uniformsSlot = ShaderGetResourceSlot(fogState.classificationShader, "VolumeFogUniforms");
	fogState.lightingTextureSlot = ShaderGetResourceSlot(fogState.classificationShader, "Lighting");

	IndexT fogIndexListsSlot = ShaderGetResourceSlot(fogState.classificationShader, "FogIndexLists");
	IndexT fogListsSlot = ShaderGetResourceSlot(fogState.classificationShader, "FogLists");

	IndexT lightIndexListsSlot = ShaderGetResourceSlot(fogState.classificationShader, "LightIndexLists");
	IndexT lightListsSlot = ShaderGetResourceSlot(fogState.classificationShader, "LightLists");
	IndexT clusterAABBSlot = ShaderGetResourceSlot(fogState.classificationShader, "ClusterAABBs");

	BufferCreateInfo rwbInfo;
	rwbInfo.name = "FogIndexListsBuffer";
	rwbInfo.size = 1;
	rwbInfo.elementSize = sizeof(Volumefog::FogIndexLists);
	rwbInfo.mode = BufferAccessMode::DeviceLocal;
	rwbInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferDestination;
	rwbInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
	fogState.clusterFogIndexLists = CreateBuffer(rwbInfo);

	rwbInfo.name = "FogLists";
	rwbInfo.elementSize = sizeof(Volumefog::FogLists);
	fogState.clusterFogLists = CreateBuffer(rwbInfo);

	rwbInfo.name = "FogListsStagingBuffer";
	rwbInfo.mode = BufferAccessMode::HostLocal;
	rwbInfo.usageFlags = CoreGraphics::TransferBufferSource;
	fogState.stagingClusterFogLists.Resize(CoreGraphics::GetNumBufferedFrames());

	fogState.cullProgram = ShaderGetProgram(fogState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Cull"));
	fogState.renderProgram = ShaderGetProgram(fogState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Render"));

	fogState.resourceTables.Resize(CoreGraphics::GetNumBufferedFrames());
	for (IndexT i = 0; i < fogState.resourceTables.Size(); i++)
	{
		fogState.resourceTables[i] = ShaderCreateResourceTable(fogState.classificationShader, NEBULA_BATCH_GROUP);
	}

	// get per-view resource tables
	const Util::FixedArray<CoreGraphics::ResourceTableId>& viewTables = TransformDevice::Instance()->GetViewResourceTables();

	for (IndexT i = 0; i < viewTables.Size(); i++)
	{
		fogState.stagingClusterFogLists[i] = CreateBuffer(rwbInfo);

		ResourceTableSetRWBuffer(viewTables[i], { fogState.clusterFogIndexLists, fogIndexListsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetRWBuffer(viewTables[i], { fogState.clusterFogLists, fogListsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetConstantBuffer(viewTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), fogState.uniformsSlot, 0, false, false, sizeof(Volumefog::VolumeFogUniforms), 0 });
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

	fogState.turbidity = 0.1f;
	fogState.color = Math::vec3(1);

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
	const Math::mat4& transform,
	const float density, 
	const Math::vec3& absorption)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	Ids::Id32 fog = fogBoxVolumeAllocator.Alloc();
	fogBoxVolumeAllocator.Set<FogBoxVolume_Transform>(fog, transform);

	fogGenericVolumeAllocator.Set<FogVolume_Type>(cid.id, BoxVolume);
	fogGenericVolumeAllocator.Set<FogVolume_TypedId>(cid.id, fog);
	fogGenericVolumeAllocator.Set<FogVolume_Turbidity>(cid.id, density);
	fogGenericVolumeAllocator.Set<FogVolume_Absorption>(cid.id, absorption);
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::SetBoxTransform(const Graphics::GraphicsEntityId id, const Math::mat4& transform)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	Ids::Id32 lid = fogGenericVolumeAllocator.Get<FogVolume_TypedId>(cid.id);
	FogVolumeType type = fogGenericVolumeAllocator.Get<FogVolume_Type>(cid.id);
	n_assert(type == FogVolumeType::BoxVolume);
	fogBoxVolumeAllocator.Set<FogBoxVolume_Transform>(lid, transform);
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::SetupSphereVolume(
	const Graphics::GraphicsEntityId id, 
	const Math::vec3& position,
	float radius, 
	const float density, 
	const Math::vec3& absorption)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	Ids::Id32 fog = fogSphereVolumeAllocator.Alloc();
	fogSphereVolumeAllocator.Set<FogSphereVolume_Position>(fog, position);
	fogSphereVolumeAllocator.Set<FogSphereVolume_Radius>(fog, radius);

	fogGenericVolumeAllocator.Set<FogVolume_Type>(cid.id, SphereVolume);
	fogGenericVolumeAllocator.Set<FogVolume_TypedId>(cid.id, fog);
	fogGenericVolumeAllocator.Set<FogVolume_Turbidity>(cid.id, density);
	fogGenericVolumeAllocator.Set<FogVolume_Absorption>(cid.id, absorption);
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::SetSpherePosition(const Graphics::GraphicsEntityId id, const Math::vec3& position)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	Ids::Id32 lid = fogGenericVolumeAllocator.Get<FogVolume_TypedId>(cid.id);
	FogVolumeType type = fogGenericVolumeAllocator.Get<FogVolume_Type>(cid.id);
	n_assert(type == FogVolumeType::SphereVolume);
	fogSphereVolumeAllocator.Set<FogSphereVolume_Position>(lid, position);
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::SetSphereRadius(const Graphics::GraphicsEntityId id, const float radius)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	Ids::Id32 lid = fogGenericVolumeAllocator.Get<FogVolume_TypedId>(cid.id);
	FogVolumeType type = fogGenericVolumeAllocator.Get<FogVolume_Type>(cid.id);
	n_assert(type == FogVolumeType::SphereVolume);
	fogSphereVolumeAllocator.Set<FogSphereVolume_Radius>(lid, radius);
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::SetTurbidity(const Graphics::GraphicsEntityId id, const float turbidity)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	fogGenericVolumeAllocator.Set<FogVolume_Turbidity>(cid.id, turbidity);
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::SetAbsorption(const Graphics::GraphicsEntityId id, const Math::vec3& absorption)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
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
	Math::mat4 viewTransform = Graphics::CameraContext::GetTransform(view->GetCamera());

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
			fogGenericVolumeAllocator.Get<FogVolume_Absorption>(i).store(fog.absorption);
			fog.turbidity = fogGenericVolumeAllocator.Get<FogVolume_Turbidity>(i);
			fog.falloff = 64.0f;
			Math::mat4 transform = fogBoxVolumeAllocator.Get<FogBoxVolume_Transform>(typeIds[i]) * viewTransform;
			Math::bbox box(transform);
			box.pmin.store3(fog.bboxMin);
			box.pmax.store3(fog.bboxMax);
			inverse(transform).store(fog.invTransform);
			numFogBoxVolumes++;
			break;
		}
		case SphereVolume:
		{
			auto& fog = fogState.fogSpheres[numFogSphereVolumes];
			fogGenericVolumeAllocator.Get<FogVolume_Absorption>(i).store(fog.absorption);
			fog.turbidity = fogGenericVolumeAllocator.Get<FogVolume_Turbidity>(i);
			Math::vec4 pos = Math::vec4(fogSphereVolumeAllocator.Get<FogSphereVolume_Position>(typeIds[i]), 1);
			pos = viewTransform * pos;
			pos.store3(fog.position);
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
		CoreGraphics::BufferUpdate(fogState.stagingClusterFogLists[bufferIndex], fogList);
		CoreGraphics::BufferFlush(fogState.stagingClusterFogLists[bufferIndex]);
	}

	Volumefog::VolumeFogUniforms fogUniforms;
	fogUniforms.NumFogBoxes = numFogBoxVolumes;
	fogUniforms.NumFogSpheres = numFogSphereVolumes;
	fogUniforms.NumVolumeFogClusters = Clustering::ClusterContext::GetNumClusters();
	fogUniforms.GlobalTurbidity = fogState.turbidity;
	fogState.color.store(fogUniforms.GlobalAbsorption);
	fogUniforms.Downscale = 4;

	CoreGraphics::TextureId fog0 = view->GetFrameScript()->GetTexture("VolumetricFogBuffer0");
	CoreGraphics::TextureId fog1 = view->GetFrameScript()->GetTexture("VolumetricFogBuffer1");

	fogState.fogVolumeTexture0 = fog0;
	fogState.fogVolumeTexture1 = fog1;
	TextureDimensions dims = TextureGetDimensions(fog0);

	ResourceTableSetRWTexture(fogState.resourceTables[bufferIndex], { fog0, fogState.lightingTextureSlot, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableCommitChanges(fogState.resourceTables[bufferIndex]);

	// get per-view resource tables
	const Util::FixedArray<CoreGraphics::ResourceTableId>& viewTables = TransformDevice::Instance()->GetViewResourceTables();

	uint offset = SetComputeConstants(MainThreadConstantBuffer, fogUniforms);
	ResourceTableSetConstantBuffer(viewTables[bufferIndex], { GetComputeConstantBuffer(MainThreadConstantBuffer), fogState.uniformsSlot, 0, false, false, sizeof(Volumefog::VolumeFogUniforms), (SizeT)offset });

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
VolumetricFogContext::RenderUI(const Graphics::FrameContext& ctx)
{
	float col[3];
	fogState.color.storeu(col);
	Shared::PerTickParams& tickParams = CoreGraphics::ShaderServer::Instance()->GetTickParams();
	if (ImGui::Begin("Volumetric Fog Params"))
	{
		ImGui::SetWindowSize(ImVec2(240, 400), ImGuiCond_Once);
		ImGui::SliderFloat("Turbidity", &fogState.turbidity, 0, 200.0f);
		ImGui::ColorEdit3("Fog Color", col);
	}
	fogState.color.loadu(col);

	ImGui::End();
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
VolumetricFogContext::SetGlobalAbsorption(const Math::vec3& color)
{
	fogState.color = color;
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4 
VolumetricFogContext::GetTransform(const Graphics::GraphicsEntityId id)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	FogVolumeType type = fogGenericVolumeAllocator.Get<FogVolume_Type>(cid.id);
	Ids::Id32 tid = fogGenericVolumeAllocator.Get<FogVolume_TypedId>(cid.id);
	switch (type)
	{
	case BoxVolume:
		return fogBoxVolumeAllocator.Get<FogBoxVolume_Transform>(tid);
	case SphereVolume:
		return Math::translation(fogSphereVolumeAllocator.Get<FogSphereVolume_Position>(tid));
	};
	return Math::mat4();
}

//------------------------------------------------------------------------------
/**
*/
void 
VolumetricFogContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::mat4& mat)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	FogVolumeType type = fogGenericVolumeAllocator.Get<FogVolume_Type>(cid.id);
	Ids::Id32 tid = fogGenericVolumeAllocator.Get<FogVolume_TypedId>(cid.id);
	switch (type)
	{
	case BoxVolume:
		return fogBoxVolumeAllocator.Set<FogBoxVolume_Transform>(tid, mat);
	case SphereVolume:
		return fogSphereVolumeAllocator.Set<FogSphereVolume_Position>(tid, xyz(mat.position));
	};
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

	CoreGraphics::BufferCopy from, to;
	from.offset = 0;
	to.offset = 0;
	Copy(ComputeQueueType, fogState.stagingClusterFogLists[bufferIndex], { from }, fogState.clusterFogLists, { to }, sizeof(Volumefog::FogLists));
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

	// set frame resources
	SetResourceTable(fogState.resourceTables[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);

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
		nullptr, 
		"Fog volume update finish barrier");

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
		nullptr, 
		"Fog volume update finish barrier");

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
