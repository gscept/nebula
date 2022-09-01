//------------------------------------------------------------------------------
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
#include "frame/framesubgraph.h"
#include "frame/framecode.h"

#include "graphics/globalconstants.h"

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

    // these are used to update the light clustering
    DecalsCluster::PBRDecal pbrDecals[256];
    DecalsCluster::EmissiveDecal emissiveDecals[256];

    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 4> frameOpAllocator;
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
    __CreateContext();
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = DecalContext::OnRenderDebug;
#endif

    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    using namespace CoreGraphics;
    decalState.classificationShader = ShaderServer::Instance()->GetShader("shd:decals_cluster.fxb");

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

    for (IndexT i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        decalState.stagingClusterDecalsList[i] = CreateBuffer(rwbInfo);

        CoreGraphics::ResourceTableId computeTable = Graphics::GetFrameResourceTableCompute(i);
        CoreGraphics::ResourceTableId graphicsTable = Graphics::GetFrameResourceTableGraphics(i);

        // update resource table
        ResourceTableSetRWBuffer(computeTable, { decalState.clusterDecalIndexLists, Shared::Table_Frame::DecalIndexLists::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetRWBuffer(computeTable, { decalState.clusterDecalsList, Shared::Table_Frame::DecalLists::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(computeTable, { CoreGraphics::GetComputeConstantBuffer(), Shared::Table_Frame::DecalUniforms::SLOT, 0, Shared::Table_Frame::DecalUniforms::SIZE, 0 });
        ResourceTableCommitChanges(computeTable);

        ResourceTableSetRWBuffer(graphicsTable, { decalState.clusterDecalIndexLists, Shared::Table_Frame::DecalIndexLists::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetRWBuffer(graphicsTable, { decalState.clusterDecalsList, Shared::Table_Frame::DecalLists::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(graphicsTable, { CoreGraphics::GetGraphicsConstantBuffer(), Shared::Table_Frame::DecalUniforms::SLOT, 0, Shared::Table_Frame::DecalUniforms::SIZE, 0 });
        ResourceTableCommitChanges(graphicsTable);
    }

    // The first pass is to copy decals over
    Frame::FrameCode* decalCopy = decalState.frameOpAllocator.Alloc<Frame::FrameCode>();
    decalCopy->domain = CoreGraphics::BarrierDomain::Global;
    decalCopy->queue = CoreGraphics::QueueType::ComputeQueueType;
    decalCopy->bufferDeps.Add(decalState.clusterDecalsList,
                                {
                                    "Decals List"
                                    , CoreGraphics::PipelineStage::TransferWrite
                                    , CoreGraphics::BufferSubresourceInfo()
                                });
    decalCopy->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::BufferCopy from, to;
        from.offset = 0;
        to.offset = 0;
        CmdCopy(cmdBuf, decalState.stagingClusterDecalsList[bufferIndex], { from }, decalState.clusterDecalsList, { to }, sizeof(DecalsCluster::DecalLists));
    };

    // The second pass is to cull the decals based on screen space AABBs
    Frame::FrameCode* decalCull = decalState.frameOpAllocator.Alloc<Frame::FrameCode>();
    decalCull->domain = CoreGraphics::BarrierDomain::Global;
    decalCull->queue = CoreGraphics::QueueType::ComputeQueueType;
    decalCull->bufferDeps.Add(decalState.clusterDecalsList,
                            {
                                "Decals List"
                                , CoreGraphics::PipelineStage::ComputeShaderRead
                                , CoreGraphics::BufferSubresourceInfo()
                            });
    decalCull->bufferDeps.Add(decalState.clusterDecalIndexLists,
                            {
                                "Decal Index Lists"
                                , CoreGraphics::PipelineStage::ComputeShaderWrite
                                , CoreGraphics::BufferSubresourceInfo()
                            });
    decalCull->bufferDeps.Add(Clustering::ClusterContext::GetClusterBuffer(),
                            {
                                "Cluster AABB"
                                , CoreGraphics::PipelineStage::ComputeShaderRead
                                , CoreGraphics::BufferSubresourceInfo()
                            });
    decalCull->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, decalState.cullProgram);

        // Run chunks of 1024 threads at a time
        std::array<SizeT, 3> dimensions = Clustering::ClusterContext::GetClusterDimensions();

        CmdDispatch(cmdBuf, Math::ceil((dimensions[0] * dimensions[1] * dimensions[2]) / 64.0f), 1, 1);
    };
    Frame::AddSubgraph("Decal Cull", { decalCopy, decalCull });

    Frame::FrameCode* pbrRender = decalState.frameOpAllocator.Alloc<Frame::FrameCode>();
    pbrRender->domain = CoreGraphics::BarrierDomain::Pass;
    pbrRender->bufferDeps.Add(decalState.clusterDecalIndexLists,
                        {
                            "Decal Index Lists"
                            , CoreGraphics::PipelineStage::PixelShaderRead
                            , CoreGraphics::BufferSubresourceInfo()
                        });
    pbrRender->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        // Set resources and draw
        CmdSetShaderProgram(cmdBuf, decalState.renderPBRProgram);
        RenderUtil::DrawFullScreenQuad::ApplyMesh(cmdBuf);
        CmdSetGraphicsPipeline(cmdBuf);
        CmdDraw(cmdBuf, RenderUtil::DrawFullScreenQuad::GetPrimitiveGroup());
    };
    Frame::AddSubgraph("PBR Decals", { pbrRender });

    Frame::FrameCode* emissiveRender = decalState.frameOpAllocator.Alloc<Frame::FrameCode>();
    emissiveRender->domain = CoreGraphics::BarrierDomain::Pass;
    emissiveRender->bufferDeps.Add(decalState.clusterDecalIndexLists,
                        {
                            "Decal Index Lists"
                            , CoreGraphics::PipelineStage::PixelShaderRead
                            , CoreGraphics::BufferSubresourceInfo()
                        });
    emissiveRender->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        // Set resources and draw
        CmdSetShaderProgram(cmdBuf, decalState.renderEmissiveProgram);
        RenderUtil::DrawFullScreenQuad::ApplyMesh(cmdBuf);
        CmdSetGraphicsPipeline(cmdBuf);
        CmdDraw(cmdBuf, RenderUtil::DrawFullScreenQuad::GetPrimitiveGroup());
    };
    Frame::AddSubgraph("Emissive Decals", { emissiveRender });
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::Discard()
{
    decalState.frameOpAllocator.Release();
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
            Math::mat4 viewSpace = viewTransform * transforms[i];
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
            Math::mat4 viewSpace = viewTransform * transforms[i];
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

    CoreGraphics::ResourceTableId computeTable = Graphics::GetFrameResourceTableCompute(bufferIndex);
    CoreGraphics::ResourceTableId graphicsTable = Graphics::GetFrameResourceTableGraphics(bufferIndex);

    uint offset = SetConstants(decalUniforms);
    ResourceTableSetConstantBuffer(computeTable, { GetComputeConstantBuffer(), Shared::Table_Frame::DecalUniforms::SLOT, 0, Shared::Table_Frame::DecalUniforms::SIZE, (SizeT)offset });
    ResourceTableCommitChanges(computeTable);
    ResourceTableSetConstantBuffer(graphicsTable, { GetGraphicsConstantBuffer(), Shared::Table_Frame::DecalUniforms::SLOT, 0, Shared::Table_Frame::DecalUniforms::SIZE, (SizeT)offset });
    ResourceTableCommitChanges(graphicsTable);

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
                RenderShape::Box, RenderShape::RenderFlag(RenderShape::CheckDepth), Math::vec4(0.8, 0.1, 0.1, 0.2), transforms[i]);

            shapeRenderer->AddShape(shape);
            break;
        }
        case EmissiveDecal:
        {
            RenderShape shape;
            shape.SetupSimpleShape(
                RenderShape::Box, RenderShape::RenderFlag(RenderShape::CheckDepth), Math::vec4(0.1, 0.8, 0.1, 0.2), transforms[i]);

            shapeRenderer->AddShape(shape);
            break;
        }
        }
    }
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
