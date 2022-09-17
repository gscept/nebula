//------------------------------------------------------------------------------
// modelcontext.cc
// (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "modelcontext.h"
#include "resources/resourceserver.h"
#include "nodes/modelnode.h"
#include "models/nodes/particlesystemnode.h"
#include "graphics/graphicsserver.h"
#include "visibility/visibilitycontext.h"
#include "profiling/profiling.h"
#include "graphics/cameracontext.h"
#include "threading/lockfreequeue.h"
#include "materials/material.h"

#include "objects_shared.h"

#ifndef PUBLIC_BUILD
#include "dynui/im3d/im3dcontext.h"
#endif

using namespace Graphics;
using namespace Resources;
namespace Models
{

ModelContext::ModelContextAllocator ModelContext::modelContextAllocator;
ModelContext::ModelInstance ModelContext::nodeInstances;
__ImplementContext(ModelContext, ModelContext::modelContextAllocator);

Threading::AtomicCounter ModelContext::ConstantsUpdateCounter = 0;
Threading::AtomicCounter ModelContext::TransformsUpdateCounter = 0;

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
        transformRange.begin = nodeInstances.transformable.nodeTransforms.Size();
        transformRange.end = nodeInstances.transformable.nodeTransforms.Size() + transformNodes.Size();
        for (SizeT i = 0; i < transformNodes.Size(); i++)
        {
            Models::TransformNode* tNode = reinterpret_cast<Models::TransformNode*>(transformNodes[i]);
            Math::transform44 trans;
            trans.setposition(tNode->position);
            trans.setrotate(tNode->rotate);
            trans.setscale(tNode->scale);
            trans.setrotatepivot(tNode->rotatePivot);
            trans.setscalepivot(tNode->scalePivot);
            nodeInstances.transformable.origTransforms.Append(trans.getmatrix());
            nodeInstances.transformable.nodeTransforms.Append(trans.getmatrix());
            if (tNode->parent != nullptr)
                nodeInstances.transformable.nodeParents.Append(nodeLookup[tNode->parent]);
            else
            {
                nodeInstances.transformable.nodeParents.Append(UINT32_MAX);
                roots.Append(i);
            }
        }

        // Setup node states
        stateRange.begin = nodeInstances.renderable.nodeStates.Size();
        stateRange.end = nodeInstances.renderable.nodeStates.Size() + renderNodes.Size();
        for (SizeT i = 0; i < renderNodes.Size(); i++)
        {
            Models::ShaderStateNode* sNode = reinterpret_cast<Models::ShaderStateNode*>(renderNodes[i]);
            NodeInstanceState state;
            state.materialInstance = CreateMaterialInstance(sNode->material);
            state.instancingConstantsIndex = sNode->instancingTransformsIndex;
            state.objectConstantsIndex = sNode->objectTransformsIndex;
            state.skinningConstantsIndex = sNode->skinningTransformsIndex;
            state.particleConstantsIndex = InvalidIndex;
            state.resourceTable = sNode->resourceTable;

            // Okay, so how this basically has to work is that there are 4 different dynamic offset constant indices in the entire engine.
            // Any change of this will break this code, so consider improving this in the future by providing a way to overload the binding point
            // for dynamic offset instead of providing several, and then simply just allocate the right type of GPU buffer for that type of node
            if (sNode->GetType() == Models::ParticleSystemNodeType)
            {
                state.particleConstantsIndex = reinterpret_cast<Models::ParticleSystemNode*>(sNode)->particleConstantsIndex;

                state.resourceTableOffsets.Resize(4);
                state.resourceTableOffsets[state.objectConstantsIndex] = 0;
                state.resourceTableOffsets[state.instancingConstantsIndex] = 0;
                state.resourceTableOffsets[state.skinningConstantsIndex] = 0;
                state.resourceTableOffsets[state.particleConstantsIndex] = 0;

            }
            else
            {
                state.resourceTableOffsets.Resize(3);
                state.resourceTableOffsets[state.objectConstantsIndex] = 0;
                state.resourceTableOffsets[state.instancingConstantsIndex] = 0;
                state.resourceTableOffsets[state.skinningConstantsIndex] = 0;
            }
            nodeInstances.renderable.nodeStates.Append(state);

            nodeInstances.renderable.nodeTransformIndex.Append(nodeLookup[renderNodes[i]]);
            nodeInstances.renderable.nodeBoundingBoxes.Append(Math::bbox());
            nodeInstances.renderable.origBoundingBoxes.Append(sNode->boundingBox);
            nodeInstances.renderable.nodeLodDistances.Append(sNode->useLodDistances ? Util::MakeTuple(sNode->minDistance, sNode->maxDistance) : Util::MakeTuple(FLT_MAX, FLT_MAX));
            nodeInstances.renderable.nodeLods.Append(0.0f);
            nodeInstances.renderable.nodeFlags.Append(Models::NodeInstanceFlags::NodeInstance_Active);
            nodeInstances.renderable.nodeMaterials.Append(sNode->material);
            nodeInstances.renderable.nodeShaderConfigs.Append(MaterialGetShaderConfig(sNode->material));
            nodeInstances.renderable.nodes.Append(sNode);
            nodeInstances.renderable.nodeModelApplyCallbacks.Append(sNode->GetApplyFunction());
            nodeInstances.renderable.modelNodeGetPrimitiveGroup.Append(sNode->GetPrimitiveGroupFunction());
            nodeInstances.renderable.nodeDrawModifiers.Append(Util::MakeTuple(1, 0)); // Base 1 instance 0 offset

            modelContextAllocator.Get<Model_NodeLookup>(cid.id).Add(sNode->GetName(), i);

#if NEBULA_GRAPHICS_DEBUG
            nodeInstances.renderable.nodeNames.Append(sNode->GetName());
#endif

            // The sort id is combined together with an index in the VisibilitySortJob to sort the node based on material, model and instance
            assert(sNode->GetSortCode() < 0xFFF0000000000000);
            assert(sNode->HashCode() < 0x000FFFFF00000000);
            uint64 sortId = ((uint64)sNode->GetSortCode() << 52) | ((uint64)sNode->HashCode() << 32);
            nodeInstances.renderable.nodeSortId.Append(sortId);
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
ModelContext::ChangeModel(const Graphics::GraphicsEntityId gfxId, const Resources::ResourceName& name, const Util::StringAtom& tag, std::function<void()> finishedCallback)
{
    const ContextEntityId cid = GetContextId(gfxId);

    Resources::ResourceId rid = modelContextAllocator.Get<Model_Id>(cid.id);

    if (rid != InvalidResourceId) // decrement model resource
        Models::DestroyModel(rid);

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
    const IndexT index = materialInstanceContexts.FindIndex(nodeInstances.renderable.nodes[node]);
    if (index == InvalidIndex)
    {
        // Lookup the batch index once
        Materials::BatchIndex batchIndex = MaterialGetBatchIndex(nodeInstances.renderable.nodeMaterials[node], batch);

        // Emplace element
        MaterialInstanceContext& ret = materialInstanceContexts.Emplace(nodeInstances.renderable.nodes[node]);
        Materials::MaterialInstanceId materialInstance = nodeInstances.renderable.nodeStates[node].materialInstance;
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
    return MaterialInstanceAllocate(nodeInstances.renderable.nodeStates[node].materialInstance, batch);
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
        nodeInstances.renderable.nodeFlags[i] = SetBits(nodeInstances.renderable.nodeFlags[i], NodeInstanceFlags::NodeInstance_AlwaysVisible);
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
    return ModelContext::nodeInstances.renderable.nodeStates;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Math::bbox>&
ModelContext::GetModelRenderableBoundingBoxes()
{
    return ModelContext::nodeInstances.renderable.nodeBoundingBoxes;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<NodeInstanceFlags>&
ModelContext::GetModelRenderableFlags()
{
    return ModelContext::nodeInstances.renderable.nodeFlags;
}

//------------------------------------------------------------------------------
/**
*/
const ModelContext::ModelInstance::Renderable&
ModelContext::GetModelRenderables()
{
    return ModelContext::nodeInstances.renderable;
}

//------------------------------------------------------------------------------
/**
*/
const ModelContext::ModelInstance::Transformable&
ModelContext::GetModelTransformables()
{
    return ModelContext::nodeInstances.transformable;
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
    Util::Array<Math::bbox>& instanceBoxes = nodeInstances.renderable.nodeBoundingBoxes;
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
                    nodeInstances.transformable.nodeTransforms[transformRange.begin + roots[j]] = transform;

                // Update transforms
                for (j = transformRange.begin + 1; j < transformRange.end; j++)
                {
                    uint32 parent = nodeInstances.transformable.nodeParents[j];
                    n_assert(parent != UINT32_MAX);
                    Math::mat4 parentTransform = nodeInstances.transformable.nodeTransforms[transformRange.begin + parent];
                    Math::mat4 orig = nodeInstances.transformable.origTransforms[j];
                    nodeInstances.transformable.nodeTransforms[j] = parentTransform * orig;
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
                Math::mat4 transform = nodeInstances.transformable.nodeTransforms[transformRange.begin + nodeInstances.renderable.nodeTransformIndex[j]];
                Math::bbox box = nodeInstances.renderable.origBoundingBoxes[j];
                box.affine_transform(transform);
                nodeInstances.renderable.nodeBoundingBoxes[j] = box;

                // calculate view vector to calculate LOD
                Math::vec4 viewVector = context->cameraTransform.position - transform.position;
                float viewDistance = length(viewVector);
                float textureLod = viewDistance - 38.5f;

                Models::NodeInstanceFlags nodeFlag = nodeInstances.renderable.nodeFlags[j];

                // Calculate if object should be culled due to LOD
                const Util::Tuple<float, float>& lodDistances = nodeInstances.renderable.nodeLodDistances[j];
                float lodFactor = 0.0f;
                if (Util::Get<0>(lodDistances) < FLT_MAX || Util::Get<1>(lodDistances) < FLT_MAX)
                {
                    lodFactor = (viewDistance - (Util::Get<0>(lodDistances) + 1.5f)) / (Util::Get<1>(lodDistances) - (Util::Get<0>(lodDistances) + 1.5f));
                    if (viewDistance >= Util::Get<0>(lodDistances) && viewDistance < Util::Get<1>(lodDistances))
                        nodeFlag = SetBits(nodeFlag, Models::NodeInstanceFlags::NodeInstance_LodActive);
                    else
                        nodeFlag = UnsetBits(nodeFlag, Models::NodeInstanceFlags::NodeInstance_LodActive);
                }
                else
                    // If not, make the lod active by default
                    nodeFlag = SetBits(nodeFlag, Models::NodeInstanceFlags::NodeInstance_LodActive);

                // Set the flags back
                nodeInstances.renderable.nodeFlags[j] = nodeFlag;

                // Set LOD factor for dithering and other shader effects
                nodeInstances.renderable.nodeLods[j] = lodFactor;

                // Notify materials system this LOD might be used (this is a bit shitty in comparison to actually using texture sampling feedback)
                Materials::MaterialSetHighestLod(nodeInstances.renderable.nodeMaterials[j], textureLod);
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
                Math::mat4 transform = nodeInstances.transformable.nodeTransforms[transformRange.begin + nodeInstances.renderable.nodeTransformIndex[j]];

                // Allocate object constants
                alignas(16) ObjectsShared::ObjectBlock block;
                transform.store(block.Model);
                inverse(transform).store(block.InvModel);
                block.DitherFactor = nodeInstances.renderable.nodeLods[j];
                block.ObjectId = j;

                uint offset = CoreGraphics::SetConstants(block);
                nodeInstances.renderable.nodeStates[j].resourceTableOffsets[nodeInstances.renderable.nodeStates[j].objectConstantsIndex] = offset;
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
    N_MARKER_BEGIN(WaitForModels, Graphics);
    ModelContext::completionEvent.Wait();
    N_MARKER_END();
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
