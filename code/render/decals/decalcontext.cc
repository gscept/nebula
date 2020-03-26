//------------------------------------------------------------------------------
//  decalcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "decalcontext.h"
#include "graphics/graphicsserver.h"

#include "decals_cluster.h"
namespace Decals
{
DecalContext::GenericDecalAllocator DecalContext::genericDecalAllocator;
DecalContext::PBRDecalAllocator DecalContext::pbrDecalAllocator;
DecalContext::EmissiveDecalAllocator DecalContext::emissiveDecalAllocator;
_ImplementContext(DecalContext, DecalContext::genericDecalAllocator);

struct
{
	CoreGraphics::ShaderId classificationShader;
	CoreGraphics::ShaderProgramId cullProgram;
	CoreGraphics::ShaderProgramId debugProgram;
	CoreGraphics::ShaderProgramId renderProgram;
	CoreGraphics::ShaderRWBufferId clusterDecalIndexLists;
	Util::FixedArray<CoreGraphics::ShaderRWBufferId> stagingClusterDecalsList;
	CoreGraphics::ShaderRWBufferId clusterDecalsList;
	CoreGraphics::ConstantBufferId clusterPointDecals;
	CoreGraphics::ConstantBufferId clusterSpotDecals;

	IndexT uniformsSlot;
	IndexT cullUniformsSlot;

	Util::FixedArray<CoreGraphics::ResourceTableId> clusterResourceTables;

	// these are used to update the light clustering
	DecalsCluster::PBRDecal pbrDecals[256];
	DecalsCluster::EmissiveDecal emissiveDecals[256];
} decalState;

//------------------------------------------------------------------------------
/**
*/
DecalContext::DecalContext()
{
}

//------------------------------------------------------------------------------
/**
*/
DecalContext::~DecalContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
DecalContext::Create()
{
	__bundle.OnUpdateViewResources = DecalContext::UpdateViewDependentResources;

#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = DecalContext::OnRenderDebug;
#endif

	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	Frame::FramePlugin::AddCallback("DecalContext - Cull and Classify", [](IndexT frame)
		{
			DecalContext::CullAndClassify();
		});

	Frame::FramePlugin::AddCallback("DecalContext - Render PBR Decals", [](IndexT frame)
		{
			DecalContext::RenderPBR();
		});

	Frame::FramePlugin::AddCallback("DecalContext - Render Emissive Decals", [](IndexT frame)
		{
			DecalContext::RenderEmissive();
		});

	using namespace CoreGraphics;
	decalState.classificationShader = ShaderServer::Instance()->GetShader("shd:decals_cluster.fxb");
	IndexT decalIndexListsSlot = ShaderGetResourceSlot(decalState.classificationShader, "DecalIndexLists");
	IndexT decalListSlot = ShaderGetResourceSlot(decalState.classificationShader, "DecalLists");
	IndexT clusterAABBSlot = ShaderGetResourceSlot(decalState.classificationShader, "ClusterAABBs");

	decalState.cullProgram = ShaderGetProgram(decalState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Cull"));
	decalState.renderProgram = ShaderGetProgram(decalState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Render"));
#ifdef CLUSTERED_DECAL_DEBUG
	decalState.debugProgram = ShaderGetProgram(decalState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Debug"));
#endif
	decalState.stagingClusterDecalsList.Resize(CoreGraphics::GetNumBufferedFrames());

	ShaderRWBufferCreateInfo rwbInfo =
	{
		"DecalIndexListsBuffer",
		sizeof(DecalsCluster::DecalIndexLists),
		BufferUpdateMode::DeviceWriteable,
		false
	};
	decalState.clusterDecalIndexLists = CreateShaderRWBuffer(rwbInfo);

	rwbInfo.name = "DecalLists";
	rwbInfo.size = sizeof(DecalsCluster::DecalLists);
	decalState.clusterDecalsList = CreateShaderRWBuffer(rwbInfo);

	rwbInfo.mode = BufferUpdateMode::HostWriteable;
	decalState.stagingClusterDecalsList.Resize(CoreGraphics::GetNumBufferedFrames());

	for (IndexT i = 0; i < decalState.clusterResourceTables.Size(); i++)
	{
		decalState.clusterResourceTables[i] = ShaderCreateResourceTable(decalState.classificationShader, NEBULA_BATCH_GROUP);
		decalState.stagingClusterDecalsList[i] = CreateShaderRWBuffer(rwbInfo);

		// update resource table
		ResourceTableSetRWBuffer(decalState.clusterResourceTables[i], { decalState.clusterDecalIndexLists, decalIndexListsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetRWBuffer(decalState.clusterResourceTables[i], { Clustering::ClusterContext::GetClusterBuffer(), clusterAABBSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetRWBuffer(decalState.clusterResourceTables[i], { decalState.clusterDecalsList, decalListSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetConstantBuffer(decalState.clusterResourceTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), decalState.uniformsSlot, 0, false, false, sizeof(DecalsCluster::ClusterUniforms), 0 });
		ResourceTableSetConstantBuffer(decalState.clusterResourceTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), decalState.cullUniformsSlot, 0, false, false, sizeof(DecalsCluster::DecalCullUniforms), 0 });
		ResourceTableCommitChanges(decalState.clusterResourceTables[i]);
	}

	_CreateContext();
}

//------------------------------------------------------------------------------
/**
*/
void 
DecalContext::Discard()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
DecalContext::SetupDecal(
	const Graphics::GraphicsEntityId id, 
	const Math::matrix44 transform, 
	const CoreGraphics::TextureId albedo, 
	const CoreGraphics::TextureId normal, 
	const CoreGraphics::TextureId material)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	genericDecalAllocator.Set<Decal_Transform>(cid.id, transform);
	Ids::Id32 decal = pbrDecalAllocator.Alloc();
	pbrDecalAllocator.Set<DecalPBR_Albedo>(decal, albedo);
	pbrDecalAllocator.Set<DecalPBR_Normal>(decal, normal);
	pbrDecalAllocator.Set<DecalPBR_Material>(decal, material);	
}

//------------------------------------------------------------------------------
/**
*/
void 
DecalContext::SetupDecal(
	const Graphics::GraphicsEntityId id, 
	const Math::matrix44 transform, 
	const CoreGraphics::TextureId emissive)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	genericDecalAllocator.Set<Decal_Transform>(cid.id, transform);
	Ids::Id32 decal = emissiveDecalAllocator.Alloc();
	emissiveDecalAllocator.Set<DecalEmissive_Emissive>(decal, emissive);
}

//------------------------------------------------------------------------------
/**
*/
void 
DecalContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44 transform)
{
	Graphics::ContextEntityId ctxId = GetContextId(id);
	genericDecalAllocator.Set<Decal_Transform>(ctxId.id, transform);
}

//------------------------------------------------------------------------------
/**
*/
void 
DecalContext::UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
	using namespace CoreGraphics;
	const Util::Array<DecalType>& types = genericDecalAllocator.GetArray<Decal_Type>();
	const Util::Array<Ids::Id32>& typeIds = genericDecalAllocator.GetArray<Decal_TypedId>();
	const Util::Array<Math::matrix44>& transforms = genericDecalAllocator.GetArray<Decal_Transform>();
	SizeT numPbrDecals = 0;
	SizeT numEmissiveDecals = 0;

	IndexT i;
	for (i = 0; i < types.Size(); i++)
	{
		switch (types[i])
		{
		case PBRDecal:
		{
			auto& pbrDecal = decalState.pbrDecals[numPbrDecals];
			Math::bbox bbox(transforms[i]);
			Math::float4::storeu(bbox.pmin, pbrDecal.bboxMin);
			Math::float4::storeu(bbox.pmax, pbrDecal.bboxMax);
			pbrDecal.Albedo = TextureGetBindlessHandle(pbrDecalAllocator.Get<DecalPBR_Albedo>(typeIds[i]));
			pbrDecal.Normal = TextureGetBindlessHandle(pbrDecalAllocator.Get<DecalPBR_Normal>(typeIds[i]));
			pbrDecal.Material = TextureGetBindlessHandle(pbrDecalAllocator.Get<DecalPBR_Material>(typeIds[i]));
			numPbrDecals++;
			break;
		}

		case EmissiveDecal:
		{
			auto& emissiveDecal = decalState.emissiveDecals[numEmissiveDecals];
			Math::bbox bbox(transforms[i]);
			Math::float4::storeu(bbox.pmin, emissiveDecal.bboxMin);
			Math::float4::storeu(bbox.pmax, emissiveDecal.bboxMax);
			emissiveDecal.Emissive = TextureGetBindlessHandle(emissiveDecalAllocator.Get<DecalEmissive_Emissive>(typeIds[i]));
			numEmissiveDecals++;
			break;
		}

		}
	}

	// setup uniforms
	DecalsCluster::DecalCullUniforms decalUniforms;
	decalUniforms.NumClusters = Clustering::ClusterContext::GetNumClusters();
	decalUniforms.NumPBRDecals = numPbrDecals;
	decalUniforms.NumEmissiveDecals = numEmissiveDecals;

	IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

	uint offset = SetComputeConstants(MainThreadConstantBuffer, decalUniforms);
	ResourceTableSetConstantBuffer(decalState.clusterResourceTables[bufferIndex], { GetComputeConstantBuffer(MainThreadConstantBuffer), decalState.cullUniformsSlot, 0, false, false, sizeof(DecalsCluster::DecalCullUniforms), (SizeT)offset });

	ClusterGenerate::ClusterUniforms clusterUniforms = Clustering::ClusterContext::GetUniforms();
	offset = SetComputeConstants(MainThreadConstantBuffer, clusterUniforms);
	ResourceTableSetConstantBuffer(decalState.clusterResourceTables[bufferIndex], { GetComputeConstantBuffer(MainThreadConstantBuffer), decalState.uniformsSlot, 0, false, false, sizeof(ClusterGenerate::ClusterUniforms), (SizeT)offset });

	// update list of point lights
	if (numPbrDecals > 0 || numEmissiveDecals > 0)
	{
		DecalsCluster::DecalLists decalList;
		Memory::CopyElements(decalState.pbrDecals, decalList.PBRDecals, numPbrDecals);
		Memory::CopyElements(decalState.emissiveDecals, decalList.EmissiveDecals, numEmissiveDecals);
		CoreGraphics::ShaderRWBufferUpdate(decalState.stagingClusterDecalsList[bufferIndex], &decalList, sizeof(DecalsCluster::DecalLists));
	}
}

//------------------------------------------------------------------------------
/**
	Todo: Right now, we just render a box, 
	but probably we want some type of mesh to illustrate this is a decal,
	and not some 'empty' object
*/
void 
DecalContext::OnRenderDebug(uint32_t flags)
{
	using namespace CoreGraphics;
	const Util::Array<DecalType>& types = genericDecalAllocator.GetArray<Decal_Type>();
	const Util::Array<Math::matrix44>& transforms = genericDecalAllocator.GetArray<Decal_Transform>();
	ShapeRenderer* shapeRenderer = ShapeRenderer::Instance();
	IndexT i;
	for (i = 0; i < types.Size(); i++)
	{
		switch (types[i])
		{
		case PBRDecal:
		{
			RenderShape shape;
			shape.SetupSimpleShape(
				RenderShape::Box, RenderShape::CheckDepth, transforms[i], Math::float4(0.8, 0, 0, 1));
				
			shapeRenderer->AddShape(shape); 
			break;
		}
		case EmissiveDecal:
		{
			RenderShape shape;
			shape.SetupSimpleShape(
				RenderShape::Box, RenderShape::CheckDepth, transforms[i], Math::float4(0, 0.8, 0, 1));

			shapeRenderer->AddShape(shape);
			break;
		}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
DecalContext::CullAndClassify()
{
	// update constants
	using namespace CoreGraphics;

	const IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

	// copy data from staging buffer to shader buffer
	Copy(ComputeQueueType, decalState.stagingClusterDecalsList[bufferIndex], 0, decalState.clusterDecalsList, 0, sizeof(DecalsCluster::DecalLists));
	BarrierInsert(ComputeQueueType,
		BarrierStage::Transfer,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				decalState.clusterDecalsList,
				BarrierAccess::TransferWrite,
				BarrierAccess::ShaderWrite,
				0, NEBULA_WHOLE_BUFFER_SIZE
			},
		}, "Decals data upload");

	// begin command buffer work
	CommandBufferBeginMarker(ComputeQueueType, NEBULA_MARKER_BLUE, "DecalsY cluster culling");

	// make sure to sync so we don't read from data that is being written...
	BarrierInsert(ComputeQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				decalState.clusterDecalIndexLists,
				BarrierAccess::ShaderRead,
				BarrierAccess::ShaderWrite,
				0, NEBULA_WHOLE_BUFFER_SIZE
			},
		}, "Decals cluster culling begin");

	SetShaderProgram(decalState.cullProgram, ComputeQueueType);
	SetResourceTable(decalState.clusterResourceTables[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr, ComputeQueueType);

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
				decalState.clusterDecalIndexLists,
				BarrierAccess::ShaderRead,
				BarrierAccess::ShaderWrite,
				0, NEBULA_WHOLE_BUFFER_SIZE
			},
		}, "Decals cluster culling end");
}

//------------------------------------------------------------------------------
/**
*/
void 
DecalContext::RenderPBR()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
DecalContext::RenderEmissive()
{
}

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId 
DecalContext::Alloc()
{
	return genericDecalAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
void 
DecalContext::Dealloc(Graphics::ContextEntityId id)
{
	Ids::Id32 typeId = genericDecalAllocator.Get<Decal_TypedId>(id.id);
	DecalContext::DecalType type = genericDecalAllocator.Get<Decal_Type>(id.id);
	switch (type)
	{
	case PBRDecal:
	{
		Resources::DiscardResource(pbrDecalAllocator.Get<DecalPBR_Albedo>(typeId));
		Resources::DiscardResource(pbrDecalAllocator.Get<DecalPBR_Normal>(typeId));
		Resources::DiscardResource(pbrDecalAllocator.Get<DecalPBR_Material>(typeId));
		pbrDecalAllocator.Dealloc(typeId);
		break;

	}
	case EmissiveDecal:
	{
		Resources::DiscardResource(emissiveDecalAllocator.Get<DecalEmissive_Emissive>(typeId));
		emissiveDecalAllocator.Dealloc(typeId);
		break;
	}
	}
}

} // namespace Decals
