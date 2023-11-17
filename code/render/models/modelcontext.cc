//------------------------------------------------------------------------------
// modelcontext.cc
// (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "modelcontext.h"
#include "resources/resourceserver.h"
#include "nodes/modelnode.h"
#include "models/nodes/primitivenode.h"
#include "graphics/graphicsserver.h"
#include "visibility/visibilitycontext.h"
#include "profiling/profiling.h"
#include "graphics/cameracontext.h"
#include "threading/lockfreequeue.h"
#include "materials/material.h"

#include "objects_shared.h"
#include "particle.h"

#ifndef PUBLIC_BUILD
#include "dynui/im3d/im3dcontext.h"
#endif

using namespace Graphics;
using namespace Resources;
namespace Models
{

ModelContext::ModelContextAllocator ModelContext::modelContextAllocator;
ModelContext::ModelInstance ModelContext::NodeInstances;
__ImplementContext(ModelContext, ModelContext::modelContextAllocator);

Threading::AtomicCounter ModelContext::ConstantsUpdateCounter = 0;
Threading::AtomicCounter ModelContext::TransformsUpdateCounter = 0;

Memory::SCAllocator ModelContext::TransformInstanceAllocator, ModelContext::RenderInstanceAllocator;

Util::Dictionary<Models::ModelNode*, ModelContext::MaterialInstanceContext> ModelContext::materialInstanceContexts;

Threading::Event ModelContext::completionEvent;

Threading::LockFreeQueue<std::function<void()>> setupCompleteQueue;

//------------------------------------------------------------------------------
/**
*/
ModelContext::ModelContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ModelContext::~ModelContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::Create()
{
    __CreateContext();

    setupCompleteQueue.Resize(65535);

#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = ModelContext::OnRenderDebug;
#endif
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    TransformInstanceAllocator = Memory::SCAllocator(0xFFFF, 0xFFFF);
    RenderInstanceAllocator = Memory::SCAllocator(0xFFFF, 0xFFFF);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::Setup(const Graphics::GraphicsEntityId gfxId, const Resources::ResourceName& name, const Util::StringAtom& tag, std::function<void()> finishedCallback)
{
    const ContextEntityId cid = GetContextId(gfxId);
    
    auto successCallback = [cid, gfxId, finishedCallback](Resources::ResourceId mid)
    {
        // Go through model nodes and setup instance data
        const Util::Array<Models::ModelNode*>& nodes = Models::ModelGetNodes(mid);

        // Run through nodes and collect transform and renderable nodes
        NodeInstanceRange& transformRange = modelContextAllocator.Get<Model_NodeInstanceTransform>(cid.id);
        NodeInstanceRange& stateRange = modelContextAllocator.Get<Model_NodeInstanceStates>(cid.id);
        Util::Array<uint32>& roots = modelContextAllocator.Get<Model_NodeInstanceRoots>(cid.id);
        Util::Array<Models::ModelNode*> transformNodes;
        Util::Array<Models::ModelNode*> renderNodes;
        Util::Array<uint32_t> nodeIds;
        Util::Dictionary<Models::ModelNode*, uint32_t> nodeLookup;
        SizeT numInstanceTransforms = 0;
        SizeT numRenderNodes = 0;
        for (SizeT i = 0; i < nodes.Size(); i++)
        {
            Models::ModelNode* node = nodes[i];
            if (node->GetBits() & Models::NodeBits::HasStateBit)
            {
                renderNodes.Append(node);

                // Setup ids for pointing state node to its transform
                nodeIds.Append(transformNodes.Size());
            }
            if (node->GetBits() & Models::NodeBits::HasTransformBit)
            {
                nodeLookup.Add(node, transformNodes.Size());
                transformNodes.Append(node);
            }
        }

        // Setup transforms
        transformRange.allocation = TransformInstanceAllocator.Alloc(transformNodes.Size());
        transformRange.begin = transformRange.allocation.offset;
        transformRange.end = transformRange.allocation.offset + transformNodes.Size();
        if (NodeInstances.transformable.nodeParents.Size() < transformRange.end)
        {
            NodeInstances.transformable.nodeParents.Resize(transformRange.end);
            NodeInstances.transformable.origTransforms.Resize(transformRange.end);
            NodeInstances.transformable.nodeTransforms.Resize(transformRange.end);
        }
        
        for (SizeT i = 0; i < transformNodes.Size(); i++)
        {
            const uint index = transformRange.allocation.offset + i;
            Models::TransformNode* tNode = reinterpret_cast<Models::TransformNode*>(transformNodes[i]);
            Math::transform44 trans;
            trans.setposition(tNode->position);
            trans.setrotate(tNode->rotate);
            trans.setscale(tNode->scale);
            trans.setrotatepivot(tNode->rotatePivot);
            trans.setscalepivot(tNode->scalePivot);
            NodeInstances.transformable.origTransforms[index] = trans.getmatrix();
            NodeInstances.transformable.nodeTransforms[index] = trans.getmatrix();
            if (tNode->parent != nullptr)
                NodeInstances.transformable.nodeParents[index] = nodeLookup[tNode->parent];
            else
            {
                NodeInstances.transformable.nodeParents[index] = UINT32_MAX;
                roots.Append(i);
            }
        }

        // Setup node states
        stateRange.allocation = RenderInstanceAllocator.Alloc(renderNodes.Size());
        stateRange.begin = stateRange.allocation.offset;
        stateRange.end = stateRange.allocation.offset + renderNodes.Size();

        if (NodeInstances.renderable.nodeStates.Size() < stateRange.end)
        {
            NodeInstances.renderable.nodeStates.Resize(stateRange.end);
            NodeInstances.renderable.nodeTransformIndex.Resize(stateRange.end);
            NodeInstances.renderable.nodeBoundingBoxes.Resize(stateRange.end);
            NodeInstances.renderable.origBoundingBoxes.Resize(stateRange.end);
            NodeInstances.renderable.nodeLodDistances.Resize(stateRange.end);
            NodeInstances.renderable.nodeLods.Resize(stateRange.end);
            NodeInstances.renderable.textureLods.Resize(stateRange.end);
            NodeInstances.renderable.nodeFlags.Resize(stateRange.end);
            NodeInstances.renderable.nodeMaterials.Resize(stateRange.end);
            NodeInstances.renderable.nodeShaderConfigs.Resize(stateRange.end);
            NodeInstances.renderable.nodeTypes.Resize(stateRange.end);
            NodeInstances.renderable.nodes.Resize(stateRange.end);
            NodeInstances.renderable.nodeMeshes.Resize(stateRange.end);
            NodeInstances.renderable.nodePrimitiveGroupIndex.Resize(stateRange.end);
            NodeInstances.renderable.nodeDrawModifiers.Resize(stateRange.end); // Base 1 instance 0 offset
            NodeInstances.renderable.nodeSortId.Resize(stateRange.end);

#if NEBULA_GRAPHICS_DEBUG
            NodeInstances.renderable.nodeNames.Resize(stateRange.end);
#endif
        }
        for (SizeT i = 0; i < renderNodes.Size(); i++)
        {
            Models::PrimitiveNode* sNode = reinterpret_cast<Models::PrimitiveNode*>(renderNodes[i]);
            NodeInstanceState state;
            state.materialInstance = CreateMaterialInstance(sNode->material);
            state.resourceTables = sNode->resourceTables;

            // Okay, so how this basically has to work is that there are 4 different dynamic offset constant indices in the entire engine.
            // Any change of this will break this code, so consider improving this in the future by providing a way to overload the binding point
            // for dynamic offset instead of providing several, and then simply just allocate the right type of GPU buffer for that type of node
            if (sNode->GetType() == Models::ParticleSystemNodeType)
            {
                state.instancingConstantsIndex = ::Particle::Table_DynamicOffset::InstancingBlock::SLOT;
                state.objectConstantsIndex = ::Particle::Table_DynamicOffset::ObjectBlock::SLOT;
                state.skinningConstantsIndex = ::Particle::Table_DynamicOffset::JointBlock::SLOT;
                state.particleConstantsIndex = ::Particle::Table_DynamicOffset::ParticleObjectBlock::SLOT;

                state.resourceTableOffsets.Resize(4);
                state.resourceTableOffsets[state.objectConstantsIndex] = 0;
                state.resourceTableOffsets[state.instancingConstantsIndex] = 0;
                state.resourceTableOffsets[state.skinningConstantsIndex] = 0;
                state.resourceTableOffsets[state.particleConstantsIndex] = 0;

            }
            else
            {
                state.instancingConstantsIndex = ObjectsShared::Table_DynamicOffset::InstancingBlock::SLOT;
                state.objectConstantsIndex = ObjectsShared::Table_DynamicOffset::ObjectBlock::SLOT;
                state.skinningConstantsIndex = ObjectsShared::Table_DynamicOffset::JointBlock::SLOT;
                state.particleConstantsIndex = InvalidIndex;

                state.resourceTableOffsets.Resize(3);
                state.resourceTableOffsets[state.objectConstantsIndex] = 0;
                state.resourceTableOffsets[state.instancingConstantsIndex] = 0;
                state.resourceTableOffsets[state.skinningConstantsIndex] = 0;
            }
            uint index = stateRange.allocation.offset + i;

            NodeInstances.renderable.nodeStates[index] = state;
            NodeInstances.renderable.nodeTransformIndex[index] = nodeLookup[renderNodes[i]];
            NodeInstances.renderable.nodeBoundingBoxes[index] = Math::bbox();
            NodeInstances.renderable.origBoundingBoxes[index] = sNode->boundingBox;
            NodeInstances.renderable.nodeLodDistances[index] = sNode->useLodDistances ? Util::MakeTuple(sNode->minDistance, sNode->maxDistance) : Util::MakeTuple(FLT_MAX, FLT_MAX);
            NodeInstances.renderable.nodeLods[index] = 0.0f;
            NodeInstances.renderable.textureLods[index] = FLT_MAX;
            NodeInstances.renderable.nodeFlags[index] = Models::NodeInstanceFlags::NodeInstance_Active;
            NodeInstances.renderable.nodeMaterials[index] = sNode->material;
            NodeInstances.renderable.nodeShaderConfigs[index] = MaterialGetShaderConfig(sNode->material);
            NodeInstances.renderable.nodeTypes[index] = sNode->GetType();
            NodeInstances.renderable.nodes[index] = sNode;
            NodeInstances.renderable.nodeMeshes[index] = sNode->GetMesh();
            NodeInstances.renderable.nodePrimitiveGroupIndex[index] = sNode->GetPrimitiveGroupIndex();
            NodeInstances.renderable.nodeDrawModifiers[index] = Util::MakeTuple(1, 0); // Base 1 instance 0 offset

            modelContextAllocator.Get<Model_NodeLookup>(cid.id).Add(sNode->GetName(), i);

            // The sort id is combined together with an index in the VisibilitySortJob to sort the node based on material, model and instance
            auto sortCode = MaterialGetSortCode(sNode->material);
            assert(sortCode < 0xFFF0000000000000);
            assert(sNode->HashCode() < 0x000FFFFF00000000);
            uint64 sortId = ((uint64)sortCode << 52) | ((uint64)sNode->HashCode() << 32);
            NodeInstances.renderable.nodeSortId[index] = sortId;

#if NEBULA_GRAPHICS_DEBUG
            NodeInstances.renderable.nodeNames[index] = sNode->GetName();
#endif
        }

        modelContextAllocator.Set<Model_Id>(cid.id, mid);
        const Math::mat4& pending = modelContextAllocator.Get<Model_Transform>(cid.id);

        // add the callbacks to a lockfree queue, and dequeue and call them when it's safe
        if (finishedCallback != nullptr)
            setupCompleteQueue.Enqueue(finishedCallback);
    };

    Resources::CreateResource(name, tag, successCallback, successCallback, false);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::Setup(
    const Graphics::GraphicsEntityId id
    , const Math::mat4 transform
    , const Math::bbox& boundingBox
    , const Materials::MaterialId material
    , const CoreGraphics::MeshId mesh
    , const IndexT primitiveGroup
#if NEBULA_GRAPHICS_DEBUG
    , const Util::String debugName
#endif
)
{
    const ContextEntityId cid = GetContextId(id);
    Util::Array<uint32>& roots = modelContextAllocator.Get<Model_NodeInstanceRoots>(cid.id);
    NodeInstanceRange& transformRange = modelContextAllocator.Get<Model_NodeInstanceTransform>(cid.id);
    NodeInstanceRange& stateRange = modelContextAllocator.Get<Model_NodeInstanceStates>(cid.id);

    // Setup transforms
    transformRange.allocation = TransformInstanceAllocator.Alloc(1);
    transformRange.begin = transformRange.allocation.offset;
    transformRange.end = transformRange.allocation.offset + 1;

    NodeInstances.transformable.origTransforms.Append(Math::mat4());
    NodeInstances.transformable.nodeTransforms.Append(transform);
    NodeInstances.transformable.nodeParents.Append(UINT32_MAX);
    roots.Append(0);

     // Setup node states
    stateRange.allocation = RenderInstanceAllocator.Alloc(1);
    stateRange.begin = stateRange.allocation.offset;
    stateRange.end = stateRange.allocation.offset + 1;

    NodeInstanceState state;
    state.materialInstance = CreateMaterialInstance(material);
    state.instancingConstantsIndex = ObjectsShared::Table_DynamicOffset::InstancingBlock::SLOT;
    state.objectConstantsIndex = ObjectsShared::Table_DynamicOffset::ObjectBlock::SLOT;
    state.skinningConstantsIndex = ObjectsShared::Table_DynamicOffset::JointBlock::SLOT;
    state.particleConstantsIndex = InvalidIndex;
    state.resourceTables = std::move(Models::ShaderStateNode::CreateResourceTables());

    state.resourceTableOffsets.Resize(3);
    state.resourceTableOffsets[state.objectConstantsIndex] = 0;
    state.resourceTableOffsets[state.instancingConstantsIndex] = 0;
    state.resourceTableOffsets[state.skinningConstantsIndex] = 0;

    NodeInstances.renderable.nodeStates.Append(state);
    NodeInstances.renderable.nodeTransformIndex.Append(0);
    NodeInstances.renderable.nodeBoundingBoxes.Append(Math::bbox());
    NodeInstances.renderable.origBoundingBoxes.Append(boundingBox);
    NodeInstances.renderable.nodeLodDistances.Append(Util::MakeTuple(FLT_MAX, FLT_MAX));
    NodeInstances.renderable.nodeLods.Append(0.0f);
    NodeInstances.renderable.textureLods.Append(1.0f);
    NodeInstances.renderable.nodeFlags.Append(Models::NodeInstanceFlags::NodeInstance_Active);
    NodeInstances.renderable.nodeMaterials.Append(material);
    NodeInstances.renderable.nodeShaderConfigs.Append(MaterialGetShaderConfig(material));
    NodeInstances.renderable.nodeTypes.Append(Models::PrimitiveNodeType);
    NodeInstances.renderable.nodes.Append(nullptr);
    NodeInstances.renderable.nodeMeshes.Append(mesh);
    NodeInstances.renderable.nodePrimitiveGroupIndex.Append(primitiveGroup);
    NodeInstances.renderable.nodeDrawModifiers.Append(Util::MakeTuple(1, 0)); // Base 1 instance 0 offset

    modelContextAllocator.Get<Model_NodeLookup>(cid.id).Add(debugName, 0);

#if NEBULA_GRAPHICS_DEBUG
    NodeInstances.renderable.nodeNames.Append(debugName);
#endif

    // The sort id is combined together with an index in the VisibilitySortJob to sort the node based on material, model and instance
    auto sortCode = Materials::MaterialGetSortCode(material);
    assert(sortCode < 0xFFF0000000000000);
    uint64 sortId = ((uint64)sortCode << 52);
    NodeInstances.renderable.nodeSortId.Append(sortId);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::ChangeModel(const Graphics::GraphicsEntityId gfxId, const Resources::ResourceName& name, const Util::StringAtom& tag, std::function<void()> finishedCallback)
{
    const ContextEntityId cid = GetContextId(gfxId);

    Resources::ResourceId rid = modelContextAllocator.Get<Model_Id>(cid.id);

    if (rid != InvalidResourceId) // decrement model resource
        Models::DestroyModel(rid);

    // Currently super broken, needs to be identical to the Setup functions
    auto successCallback = [cid, gfxId, finishedCallback](Resources::ResourceId mid)
    {
        modelContextAllocator.Get<Model_Id>(cid.id) = mid;
        const Math::mat4& pending = modelContextAllocator.Get<Model_Transform>(cid.id);
        
        if (finishedCallback != nullptr)
            setupCompleteQueue.Enqueue(finishedCallback);
    };

    Resources::ResourceId model = Resources::CreateResource(name, tag, successCallback, successCallback, true);
    modelContextAllocator.Set<Model_Id>(cid.id, model);
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelId 
ModelContext::GetModel(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return modelContextAllocator.Get<Model_Id>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelId 
ModelContext::GetModel(const Graphics::ContextEntityId id)
{
    return modelContextAllocator.Get<Model_Id>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::mat4& transform)
{
    const ContextEntityId cid = GetContextId(id);
    Math::mat4& pending = modelContextAllocator.Get<Model_Transform>(cid.id);
    bool& hasPending = modelContextAllocator.Get<Model_Dirty>(cid.id);
    pending = transform;
    hasPending = true;
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
ModelContext::GetTransform(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return modelContextAllocator.Get<Model_Transform>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
ModelContext::GetTransform(const Graphics::ContextEntityId id)
{
    return modelContextAllocator.Get<Model_Transform>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
IndexT
ModelContext::GetNodeIndex(const Graphics::GraphicsEntityId id, const Util::StringAtom& name)
{
    const ContextEntityId cid = GetContextId(id);
    const Util::Dictionary<Util::StringAtom, IndexT>& lookup = modelContextAllocator.Get<Model_NodeLookup>(cid.id);
    IndexT idx = lookup.FindIndex(name);
    n_assert(idx != InvalidIndex);
    return lookup.ValueAtIndex(idx);
}

//------------------------------------------------------------------------------
/**
*/
ModelContext::MaterialInstanceContext&
ModelContext::SetupMaterialInstanceContext(const Graphics::GraphicsEntityId id, const IndexT nodeIndex, const CoreGraphics::BatchGroup::Code batch)
{
    // This is a bit hacky, but we really need to only do this once per node and batch.
    // What we do is that we get the batch index from the batch lookup map, and the variable indexes
    const ContextEntityId cid = GetContextId(id);
    const Models::NodeInstanceRange& nodes = modelContextAllocator.Get<Model_NodeInstanceStates>(cid.id);
    const IndexT node = nodes.begin + nodeIndex;
    const IndexT index = materialInstanceContexts.FindIndex(NodeInstances.renderable.nodes[node]);
    if (index == InvalidIndex)
    {
        // Lookup the batch index once
        Materials::BatchIndex batchIndex = MaterialGetBatchIndex(NodeInstances.renderable.nodeMaterials[node], batch);

        // Emplace element
        MaterialInstanceContext& ret = materialInstanceContexts.Emplace(NodeInstances.renderable.nodes[node]);
        Materials::MaterialInstanceId materialInstance = NodeInstances.renderable.nodeStates[node].materialInstance;
        ret.batch = batchIndex;
        ret.constantBufferSize = MaterialInstanceBufferSize(materialInstance, batchIndex);
        return ret;
    }
    else
    {
        return materialInstanceContexts.ValueAtIndex(index);
    }
}

//------------------------------------------------------------------------------
/**
*/
ModelContext::MaterialInstanceContext&
ModelContext::SetupMaterialInstanceContext(const Graphics::GraphicsEntityId id, const CoreGraphics::BatchGroup::Code batch)
{
    return SetupMaterialInstanceContext(id, 0, batch);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ConstantBufferOffset
ModelContext::AllocateInstanceConstants(const Graphics::GraphicsEntityId id, const IndexT nodeIndex, const Materials::BatchIndex batch)
{
    const ContextEntityId cid = GetContextId(id);
    const Models::NodeInstanceRange& nodes = modelContextAllocator.Get<Model_NodeInstanceStates>(cid.id);
    const IndexT node = nodes.begin + nodeIndex;
    return MaterialInstanceAllocate(NodeInstances.renderable.nodeStates[node].materialInstance, batch);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ConstantBufferOffset
ModelContext::AllocateInstanceConstants(const Graphics::GraphicsEntityId id, const Materials::BatchIndex batch)
{
    return AllocateInstanceConstants(id, 0, batch);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::SetAlwaysVisible(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    const NodeInstanceRange& nodes = modelContextAllocator.Get<Model_NodeInstanceStates>(cid.id);
    for (IndexT i = nodes.begin; i < nodes.end; i++)
    {
        NodeInstances.renderable.nodeFlags[i] = SetBits(NodeInstances.renderable.nodeFlags[i], NodeInstanceFlags::NodeInstance_AlwaysVisible);
    }    
}

//------------------------------------------------------------------------------
/**
*/
const Models::NodeInstanceRange&
ModelContext::GetModelRenderableRange(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return modelContextAllocator.Get<Model_NodeInstanceStates>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const NodeInstanceRange&
ModelContext::GetModelTransformableRange(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return modelContextAllocator.Get<Model_NodeInstanceTransform>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<ModelContext::NodeInstanceState>&
ModelContext::GetModelRenderableStates()
{
    return ModelContext::NodeInstances.renderable.nodeStates;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Math::bbox>&
ModelContext::GetModelRenderableBoundingBoxes()
{
    return ModelContext::NodeInstances.renderable.nodeBoundingBoxes;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<NodeInstanceFlags>&
ModelContext::GetModelRenderableFlags()
{
    return ModelContext::NodeInstances.renderable.nodeFlags;
}

//------------------------------------------------------------------------------
/**
*/
const ModelContext::ModelInstance::Renderable&
ModelContext::GetModelRenderables()
{
    return ModelContext::NodeInstances.renderable;
}

//------------------------------------------------------------------------------
/**
*/
const ModelContext::ModelInstance::Transformable&
ModelContext::GetModelTransformables()
{
    return ModelContext::NodeInstances.transformable;
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelContext::IsLoaded(const Graphics::GraphicsEntityId id)
{
    return ModelContext::GetContextId(id) != Graphics::InvalidContextEntityId;
}

//------------------------------------------------------------------------------
/**
    Go through all models and apply their transforms
*/
void
ModelContext::UpdateTransforms(const Graphics::FrameContext& ctx)
{
    // dequeue and call all setup complete callbacks
    std::function<void()> callback;
    while (setupCompleteQueue.Dequeue(callback))
        callback();

    N_SCOPE(UpdateTransforms, Models);
    const Util::Array<NodeInstanceRange>& nodeInstanceTransformRanges = modelContextAllocator.GetArray<Model_NodeInstanceTransform>();
    const Util::Array<NodeInstanceRange>& nodeInstanceStateRanges = modelContextAllocator.GetArray<Model_NodeInstanceStates>();
    const Util::Array<Util::Array<uint32>>& nodeInstanceRoots = modelContextAllocator.GetArray<Model_NodeInstanceRoots>();
    const Util::Array<Math::bbox>& modelBoxes = Models::modelAllocator.GetArray<Model_BoundingBox>();
    Util::Array<Math::bbox>& instanceBoxes = NodeInstances.renderable.nodeBoundingBoxes;
    Util::Array<Math::mat4>& pending = modelContextAllocator.GetArray<Model_Transform>();
    Util::Array<bool>& hasPending = modelContextAllocator.GetArray<Model_Dirty>();

    // get the lod camera
    Graphics::GraphicsEntityId lodCamera = Graphics::CameraContext::GetLODCamera();
    const Math::mat4& cameraTransform = Graphics::CameraContext::GetTransform(lodCamera);

    n_assert(TransformsUpdateCounter == 0);
    TransformsUpdateCounter = 1;

    struct TransformUpdateContext
    {
        const Util::Array<NodeInstanceRange>* nodeInstanceTransformRanges;
        const Util::Array<Util::Array<uint32>>* nodeInstanceRoots;
        Util::Array<Math::mat4>* pending;
        Util::Array<bool>* hasPending;
    } transCtx;
    transCtx.nodeInstanceTransformRanges = &nodeInstanceTransformRanges;
    transCtx.nodeInstanceRoots = &nodeInstanceRoots;
    transCtx.pending = &pending;
    transCtx.hasPending = &hasPending;

    Jobs2::JobDispatch([](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
    {
        N_SCOPE(ModelTransformUpdate, Graphics);
        auto context = static_cast<TransformUpdateContext*>(ctx);
        for (IndexT i = 0; i < groupSize; i++)
        {
            IndexT index = i + invocationOffset;
            if (index >= totalJobs)
                return;

            const NodeInstanceRange& transformRange = context->nodeInstanceTransformRanges->Get(index);
            const Util::Array<uint32>& roots = context->nodeInstanceRoots->Get(index);
            if (context->hasPending->Get(index))
            {
                // The pending transform is the root of the model
                const Math::mat4 transform = context->pending->Get(index);
                context->hasPending->Get(index) = false;

                // Set root transform
                SizeT j;
                for (j = 0; j < roots.Size(); j++)
                    NodeInstances.transformable.nodeTransforms[transformRange.begin + roots[j]] = transform;

                // Update transforms
                for (j = transformRange.begin + 1; j < transformRange.end; j++)
                {
                    uint32 parent = NodeInstances.transformable.nodeParents[j];
                    n_assert(parent != UINT32_MAX);
                    Math::mat4 parentTransform = NodeInstances.transformable.nodeTransforms[transformRange.begin + parent];
                    Math::mat4 orig = NodeInstances.transformable.origTransforms[j];
                    NodeInstances.transformable.nodeTransforms[j] = parentTransform * orig;
                }
            }
        }
    }, nodeInstanceTransformRanges.Size(), 256, transCtx, nullptr, &TransformsUpdateCounter, nullptr);

    static Threading::AtomicCounter lodUpdateCounter = 0;
    n_assert(lodUpdateCounter == 0);
    lodUpdateCounter = 1;

    struct LodUpdateContext
    {
        const Util::Array<NodeInstanceRange>* nodeInstanceTransformRanges;
        const Util::Array<NodeInstanceRange>* nodeInstanceStateRanges;
        Util::Array<Math::bbox>* instanceBoxes;
        Math::mat4 cameraTransform;
    } renderCtx;
    renderCtx.nodeInstanceTransformRanges = &nodeInstanceTransformRanges;
    renderCtx.nodeInstanceStateRanges = &nodeInstanceStateRanges;
    renderCtx.instanceBoxes = &instanceBoxes;
    renderCtx.cameraTransform = cameraTransform;

    Jobs2::JobDispatch([](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
    {
        N_SCOPE(ModelLodUpdate, Graphics);
        auto context = static_cast<LodUpdateContext*>(ctx);
        for (IndexT i = 0; i < groupSize; i++)
        {
            IndexT index = i + invocationOffset;
            if (index >= totalJobs)
                return;

            const NodeInstanceRange& stateRange = context->nodeInstanceStateRanges->Get(index);
            const NodeInstanceRange& transformRange = context->nodeInstanceTransformRanges->Get(index);
            SizeT j;
            for (j = stateRange.begin; j < stateRange.end; j++)
            {
                Math::mat4 transform = NodeInstances.transformable.nodeTransforms[transformRange.begin + NodeInstances.renderable.nodeTransformIndex[j]];
                Math::bbox box = NodeInstances.renderable.origBoundingBoxes[j];
                box.affine_transform(transform);
                NodeInstances.renderable.nodeBoundingBoxes[j] = box;

                Models::PrimitiveNode* primitiveNode = static_cast<Models::PrimitiveNode*>(NodeInstances.renderable.nodes[j]);
                NodeInstances.renderable.nodeMeshes[j] = primitiveNode->GetMesh();
                NodeInstances.renderable.nodePrimitiveGroupIndex[j] = primitiveNode->GetPrimitiveGroupIndex();

                // calculate view vector to calculate LOD
                Math::vec4 viewVector = context->cameraTransform.position - transform.position;
                float viewDistance = length(viewVector);
                float textureLod = Math::max(0.0f, (viewDistance - 10.0f) / 30.5f);

                if (textureLod < NodeInstances.renderable.textureLods[j])
                {
                    // Notify materials system this LOD might be used (this is a bit shitty in comparison to actually using texture sampling feedback)
                    Materials::MaterialSetLowestLod(NodeInstances.renderable.nodeMaterials[j], textureLod);
                    NodeInstances.renderable.textureLods[j] = textureLod;
                }

                Models::NodeInstanceFlags nodeFlag = NodeInstances.renderable.nodeFlags[j];

                // Calculate if object should be culled due to LOD
                const auto& [min, max] = NodeInstances.renderable.nodeLodDistances[j];
                float lodFactor = 0.0f;
                if (min < FLT_MAX || max < FLT_MAX)
                {
                    lodFactor = (viewDistance - (min + 1.5f)) / (max - (min + 1.5f));
                    if (viewDistance >= min && viewDistance < max)
                        nodeFlag = SetBits(nodeFlag, Models::NodeInstanceFlags::NodeInstance_LodActive);
                    else
                        nodeFlag = UnsetBits(nodeFlag, Models::NodeInstanceFlags::NodeInstance_LodActive);
                }
                else
                    // If not, make the lod active by default
                    nodeFlag = SetBits(nodeFlag, Models::NodeInstanceFlags::NodeInstance_LodActive);

                // Set the flags back
                NodeInstances.renderable.nodeFlags[j] = nodeFlag;

                // Set LOD factor for dithering and other shader effects
                NodeInstances.renderable.nodeLods[j] = lodFactor;

            }
        }
    }, nodeInstanceStateRanges.Size(), 256, renderCtx, { &TransformsUpdateCounter }, &lodUpdateCounter, nullptr);

    n_assert(ConstantsUpdateCounter == 0);
    ConstantsUpdateCounter = 1;

    struct ConstantUpdateContext
    {
        const Util::Array<NodeInstanceRange>* nodeInstanceTransformRanges;
        const Util::Array<NodeInstanceRange>* nodeInstanceStateRanges;
    };

    Jobs2::JobDispatch([](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
    {
        N_SCOPE(ModelConstantUpdate, Graphics);
        auto context = static_cast<ConstantUpdateContext*>(ctx);
        for (IndexT i = 0; i < groupSize; i++)
        {
            IndexT index = i + invocationOffset;
            if (index >= totalJobs)
                return;

            const NodeInstanceRange& stateRange = context->nodeInstanceStateRanges->Get(index);
            const NodeInstanceRange& transformRange = context->nodeInstanceTransformRanges->Get(index);
            SizeT j;
            for (j = stateRange.begin; j < stateRange.end; j++)
            {
                Math::mat4 transform = NodeInstances.transformable.nodeTransforms[transformRange.begin + NodeInstances.renderable.nodeTransformIndex[j]];

                // Allocate object constants
                alignas(16) ObjectsShared::ObjectBlock block;
                transform.store(block.Model);
                inverse(transform).store(block.InvModel);
                block.DitherFactor = NodeInstances.renderable.nodeLods[j];
                block.ObjectId = j;

                uint offset = CoreGraphics::SetConstants(block);
                NodeInstances.renderable.nodeStates[j].resourceTableOffsets[NodeInstances.renderable.nodeStates[j].objectConstantsIndex] = offset;
            }
        }
    }, nodeInstanceStateRanges.Size(), 256, renderCtx, { &lodUpdateCounter }, &ConstantsUpdateCounter, &ModelContext::completionEvent);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::WaitForWork(const Graphics::FrameContext& ctx)
{
    N_SCOPE(WaitForModels, Graphics);
    ModelContext::completionEvent.Wait();
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::OnRenderDebug(uint32_t flags)
{
    //const Util::Array<ModelInstanceId>& instances = modelContextAllocator.GetArray<Model_InstanceId>();    
    //Util::Array<Math::bbox>& instanceBoxes = Models::modelPool->modelInstanceAllocator.GetArray<StreamModelCache::InstanceBoundingBox>();
    //const Util::Array<Math::mat4>& transforms = Models::modelPool->modelInstanceAllocator.GetArray<StreamModelCache::InstanceTransform>();
    //const Util::Array<Math::bbox>& modelBoxes = Models::modelPool->modelAllocator.GetArray<Model_Id>();
    //
    //Math::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
    //int i, n;
    //for (i = 0,n = instances.Size(); i<n ; i++)
    //{
    //    const ModelInstanceId& instance = instances[i];
    //    if (instance == ModelInstanceId::Invalid()) continue;
    //    CoreGraphics::RenderShape shape;
    //    shape.SetupSimpleShape(CoreGraphics::RenderShape::Box, CoreGraphics::RenderShape::RenderFlag(CoreGraphics::RenderShape::CheckDepth | CoreGraphics::RenderShape::Wireframe), transforms[instance.instance] * instanceBoxes[instance.instance].to_mat4(), white);
    //    CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
    //}
}

} // namespace Models
