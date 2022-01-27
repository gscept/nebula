﻿//------------------------------------------------------------------------------
//  decalcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "decalcontext.h"
#include "graphics/graphicsserver.h"
#include "resources/resourceserver.h"
#include "renderutil/drawfullscreenquad.h"
#include "clustering/clustercontext.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"
#include "frame/frameplugin.h"

#include "decals_cluster.h"
namespace Decals
{
DecalContext::GenericDecalAllocator DecalContext::genericDecalAllocator;
DecalContext::PBRDecalAllocator DecalContext::pbrDecalAllocator;
DecalContext::EmissiveDecalAllocator DecalContext::emissiveDecalAllocator;
__ImplementContext(DecalContext, DecalContext::genericDecalAllocator);

struct
{
    CoreGraphics::ShaderId classificationShader;
    CoreGraphics::ShaderProgramId cullProgram;
    CoreGraphics::ShaderProgramId debugProgram;
    CoreGraphics::ShaderProgramId renderPBRProgram;
    CoreGraphics::ShaderProgramId renderEmissiveProgram;
    CoreGraphics::BufferId clusterDecalIndexLists;
    Util::FixedArray<CoreGraphics::BufferId> stagingClusterDecalsList;
    CoreGraphics::BufferId clusterDecalsList;
    CoreGraphics::BufferId clusterPointDecals;
    CoreGraphics::BufferId clusterSpotDecals;

    IndexT uniformsSlot;

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

    Frame::AddCallback("DecalContext - Cull and Classify", [](const IndexT frame, const IndexT bufferIndex) {
        DecalContext::CullAndClassify();
    });

    Frame::AddCallback("DecalContext - Render PBR Decals", [](const IndexT frame, const IndexT bufferIndex) {
        DecalContext::RenderPBR();
    });

    Frame::AddCallback("DecalContext - Render Emissive Decals", [](const IndexT frame, const IndexT bufferIndex) {
        DecalContext::RenderEmissive();
    });

    using namespace CoreGraphics;
    decalState.classificationShader = ShaderServer::Instance()->GetShader("shd:decals_cluster.fxb");

    IndexT decalIndexListsSlot = ShaderGetResourceSlot(decalState.classificationShader, "DecalIndexLists");
    IndexT decalListSlot = ShaderGetResourceSlot(decalState.classificationShader, "DecalLists");

    IndexT clusterAABBSlot = ShaderGetResourceSlot(decalState.classificationShader, "ClusterAABBs");
    decalState.uniformsSlot = ShaderGetResourceSlot(decalState.classificationShader, "DecalUniforms");

    decalState.cullProgram = ShaderGetProgram(decalState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Cull"));
    decalState.renderPBRProgram = ShaderGetProgram(decalState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("RenderPBR"));
    decalState.renderEmissiveProgram = ShaderGetProgram(decalState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("RenderEmissive"));
#ifdef CLUSTERED_DECAL_DEBUG
    decalState.debugProgram = ShaderGetProgram(decalState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Debug"));
#endif

	DisplayMode mode = WindowGetDisplayMode(DisplayDevice::Instance()->GetCurrentWindow());

	BufferCreateInfo rwbInfo;
	rwbInfo.name = "DecalIndexListsBuffer";
	rwbInfo.size = 1;
	rwbInfo.elementSize = sizeof(DecalsCluster::DecalIndexLists);
	rwbInfo.mode = BufferAccessMode::DeviceLocal;
	rwbInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferDestination;
	rwbInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
	decalState.clusterDecalIndexLists = CreateBuffer(rwbInfo);

	rwbInfo.name = "DecalLists";
	rwbInfo.elementSize = sizeof(DecalsCluster::DecalLists);
	decalState.clusterDecalsList = CreateBuffer(rwbInfo);

	rwbInfo.name = "DecalListsStagingBuffer";
	rwbInfo.mode = BufferAccessMode::HostLocal;
	rwbInfo.usageFlags = CoreGraphics::TransferBufferSource;
	decalState.stagingClusterDecalsList.Resize(CoreGraphics::GetNumBufferedFrames());

	// get per-view resource tables
	const Util::FixedArray<CoreGraphics::ResourceTableId>& viewTables = TransformDevice::Instance()->GetViewResourceTables();

	for (IndexT i = 0; i < viewTables.Size(); i++)
	{
		decalState.stagingClusterDecalsList[i] = CreateBuffer(rwbInfo);

		// update resource table
		ResourceTableSetRWBuffer(viewTables[i], { decalState.clusterDecalIndexLists, decalIndexListsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetRWBuffer(viewTables[i], { decalState.clusterDecalsList, decalListSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetConstantBuffer(viewTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), decalState.uniformsSlot, 0, false, false, sizeof(DecalsCluster::DecalUniforms), 0 });
	}

	__CreateContext();
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
DecalContext::SetupDecalPBR(
    const Graphics::GraphicsEntityId id,
    const Math::mat4 transform,
    const CoreGraphics::TextureId albedo,
    const CoreGraphics::TextureId normal,
    const CoreGraphics::TextureId material)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    Ids::Id32 decal = pbrDecalAllocator.Alloc();
    pbrDecalAllocator.Set<DecalPBR_Albedo>(decal, albedo);
    pbrDecalAllocator.Set<DecalPBR_Normal>(decal, normal);
    pbrDecalAllocator.Set<DecalPBR_Material>(decal, material);

    Resources::SetMaxLOD(albedo, 0.0f, false);
    Resources::SetMaxLOD(normal, 0.0f, false);
    Resources::SetMaxLOD(material, 0.0f, false);

    genericDecalAllocator.Set<Decal_Transform>(cid.id, transform);
    genericDecalAllocator.Set<Decal_Type>(cid.id, PBRDecal);
    genericDecalAllocator.Set<Decal_TypedId>(cid.id, decal);
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::SetupDecalEmissive(
    const Graphics::GraphicsEntityId id,
    const Math::mat4 transform,
    const CoreGraphics::TextureId emissive)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    Ids::Id32 decal = emissiveDecalAllocator.Alloc();
    emissiveDecalAllocator.Set<DecalEmissive_Emissive>(decal, emissive);

    genericDecalAllocator.Set<Decal_Transform>(cid.id, transform);
    genericDecalAllocator.Set<Decal_Type>(cid.id, EmissiveDecal);
    genericDecalAllocator.Set<Decal_TypedId>(cid.id, decal);
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::SetAlbedoTexture(const Graphics::GraphicsEntityId id, const CoreGraphics::TextureId albedo)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    n_assert(genericDecalAllocator.Get<Decal_Type>(cid.id) == PBRDecal);
    Ids::Id32 decal = genericDecalAllocator.Get<Decal_TypedId>(cid.id);
    pbrDecalAllocator.Set<DecalPBR_Albedo>(decal, albedo);
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::SetNormalTexture(const Graphics::GraphicsEntityId id, const CoreGraphics::TextureId normal)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    n_assert(genericDecalAllocator.Get<Decal_Type>(cid.id) == PBRDecal);
    Ids::Id32 decal = genericDecalAllocator.Get<Decal_TypedId>(cid.id);
    pbrDecalAllocator.Set<DecalPBR_Normal>(decal, normal);
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::SetMaterialTexture(const Graphics::GraphicsEntityId id, const CoreGraphics::TextureId material)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    n_assert(genericDecalAllocator.Get<Decal_Type>(cid.id) == PBRDecal);
    Ids::Id32 decal = genericDecalAllocator.Get<Decal_TypedId>(cid.id);
    pbrDecalAllocator.Set<DecalPBR_Material>(decal, material);
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::SetEmissiveTexture(const Graphics::GraphicsEntityId id, const CoreGraphics::TextureId emissive)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    n_assert(genericDecalAllocator.Get<Decal_Type>(cid.id) == EmissiveDecal);
    Ids::Id32 decal = genericDecalAllocator.Get<Decal_TypedId>(cid.id);
    emissiveDecalAllocator.Set<DecalEmissive_Emissive>(decal, emissive);
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::mat4 transform)
{
    Graphics::ContextEntityId ctxId = GetContextId(id);
    genericDecalAllocator.Set<Decal_Transform>(ctxId.id, transform);
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
DecalContext::GetTransform(const Graphics::GraphicsEntityId id)
{
    Graphics::ContextEntityId ctxId = GetContextId(id);
    return genericDecalAllocator.Get<Decal_Transform>(ctxId.id);
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    using namespace CoreGraphics;
    Math::mat4 viewTransform = Graphics::CameraContext::GetView(view->GetCamera());
    const Util::Array<DecalType>& types = genericDecalAllocator.GetArray<Decal_Type>();
    const Util::Array<Ids::Id32>& typeIds = genericDecalAllocator.GetArray<Decal_TypedId>();
    const Util::Array<Math::mat4>& transforms = genericDecalAllocator.GetArray<Decal_Transform>();
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
            Math::mat4 viewSpace = transforms[i] * viewTransform;
            Math::bbox bbox(viewSpace);
            bbox.pmin.store(pbrDecal.bboxMin);
            bbox.pmax.store(pbrDecal.bboxMax);
            pbrDecal.albedo = TextureGetBindlessHandle(pbrDecalAllocator.Get<DecalPBR_Albedo>(typeIds[i]));
            pbrDecal.normal = TextureGetBindlessHandle(pbrDecalAllocator.Get<DecalPBR_Normal>(typeIds[i]));
            pbrDecal.material = TextureGetBindlessHandle(pbrDecalAllocator.Get<DecalPBR_Material>(typeIds[i]));
            Math::mat4 inverse = Math::inverse(transforms[i]);
            inverse.store(pbrDecal.invModel);
            transforms[i].z_axis.store3(pbrDecal.direction);
            Math::vec4 tangent = normalize(-transforms[i].x_axis);
            tangent.store3(pbrDecal.tangent);
            numPbrDecals++;
            break;
        }

        case EmissiveDecal:
        {
            auto& emissiveDecal = decalState.emissiveDecals[numEmissiveDecals];
            Math::mat4 viewSpace = transforms[i] * viewTransform;
            Math::bbox bbox(viewSpace);
            bbox.pmin.store(emissiveDecal.bboxMin);
            bbox.pmax.store(emissiveDecal.bboxMax);
            transforms[i].z_axis.store3(emissiveDecal.direction);
            emissiveDecal.emissive = TextureGetBindlessHandle(emissiveDecalAllocator.Get<DecalEmissive_Emissive>(typeIds[i]));
            numEmissiveDecals++;
            break;
        }
        }
    }
    //const CoreGraphics::TextureId normalCopyTex = view->GetFrameScript()->GetTexture("NormalBufferCopy");
    const CoreGraphics::TextureId depthTex = view->GetFrameScript()->GetTexture("ZBuffer");

    // setup uniforms
    DecalsCluster::DecalUniforms decalUniforms;
    decalUniforms.NumDecalClusters = Clustering::ClusterContext::GetNumClusters();
    decalUniforms.NumPBRDecals = numPbrDecals;
    decalUniforms.NumEmissiveDecals = numEmissiveDecals;
    //decalUniforms.NormalBufferCopy = TextureGetBindlessHandle(normalCopyTex);
    decalUniforms.StencilBuffer = TextureGetStencilBindlessHandle(depthTex);

    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

	// get per-view resource tables
	const Util::FixedArray<CoreGraphics::ResourceTableId>& viewTables = TransformDevice::Instance()->GetViewResourceTables();

    uint offset = SetComputeConstants(MainThreadConstantBuffer, decalUniforms);
    ResourceTableSetConstantBuffer(viewTables[bufferIndex], { GetComputeConstantBuffer(MainThreadConstantBuffer), decalState.uniformsSlot, 0, false, false, sizeof(DecalsCluster::DecalUniforms), (SizeT)offset });

    // update list of point lights
    if (numPbrDecals > 0 || numEmissiveDecals > 0)
    {
        DecalsCluster::DecalLists decalList;
        Memory::CopyElements(decalState.pbrDecals, decalList.PBRDecals, numPbrDecals);
        Memory::CopyElements(decalState.emissiveDecals, decalList.EmissiveDecals, numEmissiveDecals);
        CoreGraphics::BufferUpdate(decalState.stagingClusterDecalsList[bufferIndex], decalList);
        CoreGraphics::BufferFlush(decalState.stagingClusterDecalsList[bufferIndex]);
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
    const Util::Array<Math::mat4>& transforms = genericDecalAllocator.GetArray<Decal_Transform>();
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
                RenderShape::Box, RenderShape::RenderFlag(RenderShape::CheckDepth), transforms[i], Math::vec4(0.8, 0.1, 0.1, 0.2));

            shapeRenderer->AddShape(shape);
            break;
        }
        case EmissiveDecal:
        {
            RenderShape shape;
            shape.SetupSimpleShape(
                RenderShape::Box, RenderShape::RenderFlag(RenderShape::CheckDepth), transforms[i], Math::vec4(0.1, 0.8, 0.1, 0.2));

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
    BarrierInsert(ComputeQueueType,
                  BarrierStage::ComputeShader,
                  BarrierStage::Transfer,
                  BarrierDomain::Global,
                  nullptr,
                  {
                      BufferBarrier {
                          decalState.clusterDecalsList,
                          BarrierAccess::ShaderRead,
                          BarrierAccess::TransferWrite,
                          0, NEBULA_WHOLE_BUFFER_SIZE },
                  },
                  "Decals data upload");

    CoreGraphics::BufferCopy from, to;
    from.offset = 0;
    to.offset = 0;
    Copy(ComputeQueueType, decalState.stagingClusterDecalsList[bufferIndex], { from }, decalState.clusterDecalsList, { to }, sizeof(DecalsCluster::DecalLists));
    BarrierInsert(ComputeQueueType,
                  BarrierStage::Transfer,
                  BarrierStage::ComputeShader,
                  BarrierDomain::Global,
                  nullptr,
                  {
                      BufferBarrier {
                          decalState.clusterDecalsList,
                          BarrierAccess::TransferWrite,
                          BarrierAccess::ShaderRead,
                          0, NEBULA_WHOLE_BUFFER_SIZE },
                  },
                  "Decals data upload");

    // begin command buffer work
    CommandBufferBeginMarker(ComputeQueueType, NEBULA_MARKER_BLUE, "Decals cluster culling");

    // make sure to sync so we don't read from data that is being written...
    BarrierInsert(ComputeQueueType,
                  BarrierStage::ComputeShader,
                  BarrierStage::ComputeShader,
                  BarrierDomain::Global,
                  nullptr,
                  {
                      BufferBarrier {
                          decalState.clusterDecalIndexLists,
                          BarrierAccess::ShaderRead,
                          BarrierAccess::ShaderWrite,
                          0, NEBULA_WHOLE_BUFFER_SIZE },
                  },
                  "Decals cluster culling begin");

    SetShaderProgram(decalState.cullProgram, ComputeQueueType);

    // run chunks of 1024 threads at a time
    std::array<SizeT, 3> dimensions = Clustering::ClusterContext::GetClusterDimensions();

    Compute(Math::ceil((dimensions[0] * dimensions[1] * dimensions[2]) / 64.0f), 1, 1, ComputeQueueType);

    // make sure to sync so we don't read from data that is being written...
    BarrierInsert(ComputeQueueType,
                  BarrierStage::ComputeShader,
                  BarrierStage::ComputeShader,
                  BarrierDomain::Global,
                  nullptr,
                  {
                      BufferBarrier {
                          decalState.clusterDecalIndexLists,
                          BarrierAccess::ShaderWrite,
                          BarrierAccess::ShaderRead,
                          0, NEBULA_WHOLE_BUFFER_SIZE },
                  },
                  "Decals cluster culling end");

    CommandBufferEndMarker(ComputeQueueType);
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::RenderPBR()
{
    // update constants
    using namespace CoreGraphics;

    const IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    // begin a new batch (not sure if needed)
    BeginBatch(Frame::FrameBatchType::System);

    // set resources and draw
    SetShaderProgram(decalState.renderPBRProgram);
    RenderUtil::DrawFullScreenQuad::ApplyMesh();
    Draw();

    // end the batch
    EndBatch();
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::RenderEmissive()
{
    // update constants
    using namespace CoreGraphics;

    const IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    // begin a new batch (not sure if needed)
    BeginBatch(Frame::FrameBatchType::System);

    // set resources and draw
    SetShaderProgram(decalState.renderEmissiveProgram);
    RenderUtil::DrawFullScreenQuad::ApplyMesh();
    SetGraphicsPipeline();
    Draw();

    // end the batch
    EndBatch();
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
