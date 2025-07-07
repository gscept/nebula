//------------------------------------------------------------------------------
//  decalcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

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

#include "system_shaders/decals_cluster.h"

#include "frame/default.h"

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
    CoreGraphics::BufferSet stagingClusterDecalsList;
    CoreGraphics::BufferId clusterDecalsList;
    CoreGraphics::BufferId clusterPointDecals;
    CoreGraphics::BufferId clusterSpotDecals;

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
    __CreateContext();
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = DecalContext::OnRenderDebug;
#endif

    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    using namespace CoreGraphics;
    decalState.classificationShader = CoreGraphics::ShaderGet("shd:system_shaders/decals_cluster.fxb");

    decalState.cullProgram = ShaderGetProgram(decalState.classificationShader, CoreGraphics::ShaderFeatureMask("Cull"));
    decalState.renderPBRProgram = ShaderGetProgram(decalState.classificationShader, CoreGraphics::ShaderFeatureMask("RenderPBR"));
    decalState.renderEmissiveProgram = ShaderGetProgram(decalState.classificationShader, CoreGraphics::ShaderFeatureMask("RenderEmissive"));
#ifdef CLUSTERED_DECAL_DEBUG
    decalState.debugProgram = ShaderGetProgram(decalState.classificationShader, CoreGraphics::ShaderFeatureMask("Debug"));
#endif
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
    decalState.stagingClusterDecalsList = BufferSet(rwbInfo);

    for (IndexT i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        CoreGraphics::ResourceTableId frameResourceTable = Graphics::GetFrameResourceTable(i);

        ResourceTableSetRWBuffer(frameResourceTable, { decalState.clusterDecalIndexLists, Shared::Table_Frame::DecalIndexLists_SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetRWBuffer(frameResourceTable, { decalState.clusterDecalsList, Shared::Table_Frame::DecalLists_SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(frameResourceTable, { CoreGraphics::GetConstantBuffer(i), Shared::Table_Frame::DecalUniforms_SLOT, 0, sizeof(Shared::DecalUniforms), 0 });
        ResourceTableCommitChanges(frameResourceTable);
    }

    FrameScript_default::Bind_ClusterDecalList(decalState.clusterDecalsList);
    FrameScript_default::Bind_ClusterDecalIndexLists(decalState.clusterDecalIndexLists);
    FrameScript_default::RegisterSubgraph_DecalCopy_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::BufferCopy from, to;
        from.offset = 0;
        to.offset = 0;
        CmdCopy(cmdBuf, decalState.stagingClusterDecalsList.buffers[bufferIndex], { from }, decalState.clusterDecalsList, { to }, sizeof(DecalsCluster::DecalLists));
    }, {
        { FrameScript_default::BufferIndex::ClusterDecalList, CoreGraphics::PipelineStage::TransferWrite }
    });

    FrameScript_default::RegisterSubgraph_DecalCull_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, decalState.cullProgram);

        // Run chunks of 1024 threads at a time
        std::array<SizeT, 3> dimensions = Clustering::ClusterContext::GetClusterDimensions();

        CmdDispatch(cmdBuf, Math::ceil((dimensions[0] * dimensions[1] * dimensions[2]) / 64.0f), 1, 1);
    }, {
        { FrameScript_default::BufferIndex::ClusterDecalList, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::BufferIndex::ClusterDecalIndexLists, CoreGraphics::PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::BufferIndex::ClusterBuffer, CoreGraphics::PipelineStage::ComputeShaderRead }
    });
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
    const Resources::ResourceId albedo,
    const Resources::ResourceId normal,
    const Resources::ResourceId material)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    Ids::Id32 decal = pbrDecalAllocator.Alloc();
    pbrDecalAllocator.Set<DecalPBR_Albedo>(decal, albedo);
    pbrDecalAllocator.Set<DecalPBR_Normal>(decal, normal);
    pbrDecalAllocator.Set<DecalPBR_Material>(decal, material);

    Resources::SetMinLod(albedo, 0.0f, false);
    Resources::SetMinLod(normal, 0.0f, false);
    Resources::SetMinLod(material, 0.0f, false);

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
    const Resources::ResourceId emissive)
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
DecalContext::SetAlbedoTexture(const Graphics::GraphicsEntityId id, const Resources::ResourceId albedo)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    if (cid == Graphics::ContextEntityId::Invalid())
        return;

    n_assert(genericDecalAllocator.Get<Decal_Type>(cid.id) == PBRDecal);
    Ids::Id32 decal = genericDecalAllocator.Get<Decal_TypedId>(cid.id);
    pbrDecalAllocator.Set<DecalPBR_Albedo>(decal, albedo);
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::SetNormalTexture(const Graphics::GraphicsEntityId id, const Resources::ResourceId normal)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    if (cid == Graphics::ContextEntityId::Invalid())
        return;

    n_assert(genericDecalAllocator.Get<Decal_Type>(cid.id) == PBRDecal);
    Ids::Id32 decal = genericDecalAllocator.Get<Decal_TypedId>(cid.id);
    pbrDecalAllocator.Set<DecalPBR_Normal>(decal, normal);
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::SetMaterialTexture(const Graphics::GraphicsEntityId id, const Resources::ResourceId material)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    if (cid == Graphics::ContextEntityId::Invalid())
        return;

    n_assert(genericDecalAllocator.Get<Decal_Type>(cid.id) == PBRDecal);
    Ids::Id32 decal = genericDecalAllocator.Get<Decal_TypedId>(cid.id);
    pbrDecalAllocator.Set<DecalPBR_Material>(decal, material);
}

//------------------------------------------------------------------------------
/**
*/
void
DecalContext::SetEmissiveTexture(const Graphics::GraphicsEntityId id, const Resources::ResourceId emissive)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    if (cid == Graphics::ContextEntityId::Invalid())
        return;

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
    if (ctxId == Graphics::ContextEntityId::Invalid())
        return;

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

    // setup uniforms
    DecalsCluster::DecalUniforms decalUniforms;
    decalUniforms.NumDecalClusters = Clustering::ClusterContext::GetNumClusters();
    decalUniforms.NumPBRDecals = numPbrDecals;
    decalUniforms.NumEmissiveDecals = numEmissiveDecals;
    //decalUniforms.NormalBufferCopy = TextureGetBindlessHandle(normalCopyTex);
    decalUniforms.StencilBuffer = TextureGetStencilBindlessHandle(FrameScript_default::Texture_ZBuffer());

    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    CoreGraphics::ResourceTableId frameResourceTable = Graphics::GetFrameResourceTable(bufferIndex);

    uint64_t offset = SetConstants(decalUniforms);
    ResourceTableSetConstantBuffer(frameResourceTable, { GetConstantBuffer(bufferIndex), Shared::Table_Frame::DecalUniforms_SLOT, 0, sizeof(Shared::DecalUniforms), offset });
    ResourceTableCommitChanges(frameResourceTable);

    // update list of point lights
    if (numPbrDecals > 0 || numEmissiveDecals > 0)
    {
        DecalsCluster::DecalLists decalList;
        Memory::CopyElements(decalState.pbrDecals, decalList.PBRDecals, numPbrDecals);
        Memory::CopyElements(decalState.emissiveDecals, decalList.EmissiveDecals, numEmissiveDecals);
        CoreGraphics::BufferUpdate(decalState.stagingClusterDecalsList.buffers[bufferIndex], decalList);
        CoreGraphics::BufferFlush(decalState.stagingClusterDecalsList.buffers[bufferIndex]);
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
