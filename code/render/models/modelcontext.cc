//------------------------------------------------------------------------------
// modelcontext.cc
// (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "modelcontext.h"
#include "resources/resourceserver.h"
#include "nodes/modelnode.h"
#include "streammodelpool.h"
#include "graphics/graphicsserver.h"
#include "visibility/visibilitycontext.h"
#include "profiling/profiling.h"
#include "graphics/cameracontext.h"
#include "threading/lockfreequeue.h"

#ifndef PUBLIC_BUILD
#include "dynui/im3d/im3dcontext.h"
#endif

using namespace Graphics;
using namespace Resources;
namespace Models
{

ModelContext::ModelContextAllocator ModelContext::modelContextAllocator;
ModelContext::ModelInstance ModelContext::nodeInstances;
Jobs::JobSyncId ModelContext::jobInternalSync;
Jobs::JobSyncId ModelContext::jobHostSync;
__ImplementContext(ModelContext, ModelContext::modelContextAllocator);

Threading::LockFreeQueue<std::function<void()>> setupCompleteQueue;

extern void ModelBoundingBoxUpdateJob(const Jobs::JobFuncContext& ctx);
extern void ModelTransformUpdateJob(const Jobs::JobFuncContext& ctx);

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

    __bundle.OnBegin = ModelContext::UpdateTransforms;
    __bundle.StageBits = &ModelContext::__state.currentStage;
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = ModelContext::OnRenderDebug;
#endif
    ModelContext::__state.allowedRemoveStages = Graphics::OnBeforeFrameStage;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    ModelContext::jobInternalSync = Jobs::CreateJobSync({ nullptr });
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::Setup(const Graphics::GraphicsEntityId gfxId, const Resources::ResourceName& name, const Util::StringAtom& tag, std::function<void()> finishedCallback)
{
    const ContextEntityId cid = GetContextId(gfxId);
    modelContextAllocator.Get<Model_InstanceId>(cid.id) = ModelInstanceId::Invalid();
    
    ResourceCreateInfo info;
    info.resource = name;
    info.tag = tag;
    info.async = true;
    info.successCallback = [cid, gfxId, finishedCallback](Resources::ResourceId mid)
    {
        // Go through model nodes and setup instance data
        const Util::Dictionary<Util::StringAtom, Models::ModelNode*>& nodes = Models::ModelGetNodes(mid);

        // Run through nodes and collect transform and renderable nodes
        NodeInstanceRange& transformRange = modelContextAllocator.Get<Model_NodeInstanceTransform>(cid.id);
        NodeInstanceRange& stateRange = modelContextAllocator.Get<Model_NodeInstanceStates>(cid.id);
        Util::Array<Models::ModelNode*> transformNodes;
        Util::Array<Models::ModelNode*> renderNodes;
        Util::Array<uint32_t> nodeIds;
        Util::Dictionary<Models::ModelNode*, uint32_t> nodeLookup;
        SizeT numInstanceTransforms = 0;
        SizeT numRenderNodes = 0;
        for (SizeT i = 0; i < nodes.Size(); i++)
        {
            Models::ModelNode* node = nodes.ValueAtIndex(i);
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
        transformRange.offset = nodeInstances.transformable.nodeTransforms.Size();
        transformRange.size = transformNodes.Size();
        nodeInstances.transformable.nodeTransforms.Reserve(transformNodes.Size());
        nodeInstances.transformable.nodeParents.Reserve(transformNodes.Size());
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
            nodeInstances.transformable.nodeParents.Append(nodeLookup[tNode->parent]);
        }

        // Setup node states
        stateRange.offset = nodeInstances.renderable.nodeStates.Size();
        stateRange.size = renderNodes.Size();
        nodeInstances.renderable.nodeStates.Reserve(renderNodes.Size());
        nodeInstances.renderable.nodeBoundingBoxes.Reserve(renderNodes.Size());
        nodeInstances.renderable.nodeFlags.Reserve(renderNodes.Size());
        nodeInstances.renderable.nodeTransformIndex = nodeIds;
        for (SizeT i = 0; i < renderNodes.Size(); i++)
        {
            Models::ShaderStateNode* sNode = reinterpret_cast<Models::ShaderStateNode*>(renderNodes[i]);
            NodeInstanceState state;
            state.surfaceInstance = sNode->materialType->CreateSurfaceInstance(sNode->surface);
            state.instancingConstantsIndex = sNode->instancingTransformsIndex;
            state.objectConstantsIndex = sNode->objectTransformsIndex;
            state.skinningConstantsIndex = sNode->skinningTransformsIndex;
            state.resourceTable = sNode->resourceTable;
            state.resourceTableOffsets.Resize(3);
            state.resourceTableOffsets[state.objectConstantsIndex] = 0;
            state.resourceTableOffsets[state.instancingConstantsIndex] = 0;
            state.resourceTableOffsets[state.skinningConstantsIndex] = 0;
            nodeInstances.renderable.nodeStates.Append(state);

            nodeInstances.renderable.nodeBoundingBoxes.Append(Math::bbox());
            nodeInstances.renderable.origBoundingBoxes.Append(sNode->boundingBox);
            nodeInstances.renderable.nodeLodDistances.Append(sNode->useLodDistances ? Util::MakeTuple(sNode->minDistance, sNode->maxDistance) : Util::MakeTuple(FLT_MAX, FLT_MAX));
            nodeInstances.renderable.nodeLods.Append(0.0f);
            nodeInstances.renderable.nodeFlags.Append(Models::NodeInstance_Active);
            nodeInstances.renderable.nodeSurfaceResources.Append(sNode->surRes);
            nodeInstances.renderable.nodeMaterialTypes.Append(sNode->materialType);
            nodeInstances.renderable.nodes.Append(sNode);

            assert(sNode->materialType->HashCode() < 0xFFF0000000000000);
            assert(sNode->HashCode() < 0x000FFFFF00000000);
            uint64 sortId = ((uint64)sNode->materialType->HashCode() << 52) | ((uint64)sNode->HashCode() << 32);
        }
        

        modelContextAllocator.Get<Model_Id>(cid.id) = mid;
        ModelInstanceId& mdl = modelContextAllocator.Get<Model_InstanceId>(cid.id);
        mdl = Models::CreateModelInstance(mid);
        const Math::mat4& pending = modelContextAllocator.Get<Model_Transform>(cid.id);
        Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::InstanceTransform>(mdl.instance) = pending;
        Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ObjectId>(mdl.instance) = gfxId.id;

        // add the callbacks to a lockfree queue, and dequeue and call them when it's safe
        if (finishedCallback != nullptr)
            setupCompleteQueue.Enqueue(finishedCallback);
    };
    info.failCallback = info.successCallback;

    ModelId mid = Models::CreateModel(info);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::ChangeModel(const Graphics::GraphicsEntityId gfxId, const Resources::ResourceName& name, const Util::StringAtom& tag, std::function<void()> finishedCallback)
{
    const ContextEntityId cid = GetContextId(gfxId);

    // clean up old stuff, but don't deallocate entity
    ModelId& rid = modelContextAllocator.Get<Model_Id>(cid.id);
    ModelInstanceId& mdl = modelContextAllocator.Get<Model_InstanceId>(cid.id);

    if (mdl != ModelInstanceId::Invalid()) // actually deallocate current instance
        Models::DestroyModelInstance(mdl);
    if (rid != ModelId::Invalid()) // decrement model resource
        Models::DestroyModel(rid);
    mdl = ModelInstanceId::Invalid();

    ResourceCreateInfo info;
    info.resource = name;
    info.tag = tag;
    info.async = false;
    info.successCallback = [&mdl, cid, gfxId, finishedCallback](Resources::ResourceId mid)
    {
        modelContextAllocator.Get<Model_Id>(cid.id) = mid;
        ModelInstanceId& mdl = modelContextAllocator.Get<Model_InstanceId>(cid.id);
        const Math::mat4& pending = modelContextAllocator.Get<Model_Transform>(cid.id);
        Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::InstanceTransform>(mdl.instance) = pending;
        Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ObjectId>(mdl.instance) = gfxId.id;
        
        if (finishedCallback != nullptr)
            setupCompleteQueue.Enqueue(finishedCallback);
    };
    info.failCallback = info.successCallback;

    rid = Models::CreateModel(info);
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
const Models::ModelInstanceId
ModelContext::GetModelInstance(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return modelContextAllocator.Get<Model_InstanceId>(cid.id);
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
const Models::ModelInstanceId
ModelContext::GetModelInstance(const Graphics::ContextEntityId id)
{
    return modelContextAllocator.Get<Model_InstanceId>(id.id);
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
    ModelInstanceId& inst = modelContextAllocator.Get<Model_InstanceId>(id.id);
    return modelContextAllocator.Get<Model_Transform>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
Math::bbox 
ModelContext::GetBoundingBox(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    ModelInstanceId& inst = modelContextAllocator.Get<Model_InstanceId>(cid.id);
    return Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::InstanceBoundingBox>(inst.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::ModelNode::Instance*>& 
ModelContext::GetModelNodeInstances(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    ModelInstanceId& inst = modelContextAllocator.Get<Model_InstanceId>(cid.id);
    return Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ModelNodeInstances>(inst.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::NodeType>& 
ModelContext::GetModelNodeTypes(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    ModelInstanceId& inst = modelContextAllocator.Get<Model_InstanceId>(cid.id);
    return Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ModelNodeTypes>(inst.instance);
}

//------------------------------------------------------------------------------
/**
*/
const ModelContext::NodeInstanceRange&
ModelContext::GetModelNodeInstanceStateRange(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return modelContextAllocator.Get<Model_NodeInstanceStates>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<ModelContext::NodeInstanceState>&
ModelContext::GetModelNodeInstanceStates()
{
    return ModelContext::nodeInstances.renderable.nodeStates;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Math::bbox>&
ModelContext::GetModelNodeInstanceBoundingBoxes()
{
    return ModelContext::nodeInstances.renderable.nodeBoundingBoxes;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<NodeInstanceFlags>&
ModelContext::GetModelNodeInstanceFlags()
{
    return ModelContext::nodeInstances.renderable.nodeFlags;
}

//------------------------------------------------------------------------------
/**
*/
const ModelContext::ModelInstance::Renderable&
ModelContext::GetModelNodeInstanceRenderables()
{
    return ModelContext::nodeInstances.renderable;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::ModelNode::Instance*>&
ModelContext::GetModelNodeInstances(const Graphics::ContextEntityId id)
{
    ModelInstanceId& inst = modelContextAllocator.Get<Model_InstanceId>(id.id);
    return Models::modelPool->modelInstanceAllocator.Get<Model_Id>(inst.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::NodeType>&
ModelContext::GetModelNodeTypes(const Graphics::ContextEntityId id)
{
    ModelInstanceId& inst = modelContextAllocator.Get<Model_InstanceId>(id.id);
    return Models::modelPool->modelInstanceAllocator.Get<Models::StreamModelPool::ModelNodeTypes>(inst.instance);
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
    const Util::Array<ModelInstanceId>& instances = modelContextAllocator.GetArray<Model_InstanceId>();
    const Util::Array<NodeInstanceRange>& nodeInstanceTransformRanges = modelContextAllocator.GetArray<Model_NodeInstanceTransform>();
    const Util::Array<NodeInstanceRange>& nodeInstanceStateRanges = modelContextAllocator.GetArray<Model_NodeInstanceStates>();
    const Util::Array<Math::bbox>& modelBoxes = Models::modelPool->modelAllocator.GetArray<Models::StreamModelPool::ModelBoundingBox>();
    Util::Array<Math::bbox>& instanceBoxes = nodeInstances.renderable.nodeBoundingBoxes;
    Util::Array<Math::mat4>& pending = modelContextAllocator.GetArray<Model_Transform>();
    Util::Array<bool>& hasPending = modelContextAllocator.GetArray<Model_Dirty>();

    // get the lod camera
    Graphics::GraphicsEntityId lodCamera = Graphics::CameraContext::GetLODCamera();
    const Math::mat4& cameraTransform = inverse(Graphics::CameraContext::GetTransform(lodCamera));

    // TODO: Make each of these loops into a job and put them all in a job chain


    // Go through transforms and perform hierarchical multiplication
    SizeT i;
    for (i = 0; i < nodeInstanceTransformRanges.Size(); i++)
    {
        const NodeInstanceRange& transformRange = nodeInstanceTransformRanges[i];
        if (hasPending[i])
        {
            // The pending transform is the root of the model
            const Math::mat4 transform = pending[i];
            hasPending[i] = false;

            // Set root transform
            nodeInstances.transformable.nodeTransforms[transformRange.offset] = transform;

            // Update transforms
            SizeT j;
            for (j = 1; j < transformRange.size; j++)
            {
                uint32_t parent = nodeInstances.transformable.nodeParents[transformRange.offset + j];
                Math::mat4 parentTransform = nodeInstances.transformable.nodeTransforms[parent];
                Math::mat4 orig = nodeInstances.transformable.origTransforms[transformRange.offset + j];
                nodeInstances.transformable.nodeTransforms[transformRange.offset + j] = orig * parentTransform;
            }
        }
    }

    /* Setup model node state bounding box update
    Jobs::JobContext jctx;
    jctx.uniform.scratchSize = 0;
    jctx.uniform.numBuffers = 1;
    jctx.uniform.data[0] = &nodeInstances.renderable;
    jctx.uniform.dataSize[0] = sizeof(&nodeInstances.renderable);
    jctx.input.numBuffers = 1;
    jctx.input.data[0] = nodeInstanceStateRanges.Begin();
    jctx.input.dataSize[0] = nodeInstanceStateRanges.Size() * sizeof(NodeInstanceRange);
    jctx.input.sliceSize[0] = nodeInstanceStateRanges.Size() * sizeof(NodeInstanceRange);
    jctx.output.numBuffers = 1;
    jctx.output.data[0] = jctx.input.data[0];
    jctx.output.sliceSize[0] = jctx.input.dataSize[0];
    Jobs::JobId job = Jobs::CreateJob({ ModelBoundingBoxUpdateJob });
    Jobs::JobSchedule(job, Graphics::GraphicsServer::renderSystemsJobPort, jctx);

    // Issue sync and wait on the threads
    Jobs::JobSyncSignal(ModelContext::jobInternalSync, Graphics::GraphicsServer::renderSystemsJobPort, false);
    Jobs::JobSyncThreadWait(ModelContext::jobInternalSync, Graphics::GraphicsServer::renderSystemsJobPort, true);
    */

    // Now go through state nodes and update their bounding boxes
    for (i = 0; i < nodeInstanceStateRanges.Size(); i++)
    {
        const NodeInstanceRange& stateRange = nodeInstanceStateRanges[i];
        SizeT j;
        for (j = 0; j < stateRange.size; j++)
        {
            Math::mat4 transform = nodeInstances.transformable.nodeTransforms[nodeInstances.renderable.nodeTransformIndex[stateRange.offset + j]];
            Math::bbox box = nodeInstances.renderable.origBoundingBoxes[stateRange.offset + j];
            box.affine_transform(transform);
            nodeInstances.renderable.nodeBoundingBoxes[stateRange.offset + j] = box;

            // calculate view vector to calculate LOD
            Math::vec4 viewVector = cameraTransform.position - transform.position;
            float viewDistance = length(viewVector);
            float textureLod = viewDistance - 38.5f;

            Models::NodeInstanceFlags& nodeFlag = nodeInstances.renderable.nodeFlags[stateRange.offset + j];

            // Calculate if object should be culled due to LOD
            const Util::Tuple<float, float>& lodDistances = nodeInstances.renderable.nodeLodDistances[stateRange.offset + j];
            if (viewDistance >= Util::Get<0>(lodDistances) && viewDistance < Util::Get<1>(lodDistances))
                nodeFlag = SetBits(nodeFlag, Models::NodeInstance_LodActive);
            else
                nodeFlag = UnsetBits(nodeFlag, Models::NodeInstance_LodActive);

            // Set LOD factor
            nodeInstances.renderable.nodeLods[stateRange.offset + j] = (viewDistance - (Util::Get<0>(lodDistances) + 1.5f)) / (Util::Get<1>(lodDistances) - (Util::Get<0>(lodDistances) + 1.5f));

            // Notify materials system this LOD might be used
            Materials::surfacePool->SetMaxLOD(nodeInstances.renderable.nodeSurfaceResources[stateRange.offset + j], textureLod);
        }
    }
    /*
    SizeT i;
    for (i = 0; i < instances.Size(); i++)
    {
        const ModelInstanceId& instance = instances[i];
        if (instance == ModelInstanceId::Invalid()) 
            continue; // hmm, bad, should reorder and keep a 'last valid index'

        Util::Array<Models::ModelNode::Instance*>& nodes = Models::modelPool->modelInstanceAllocator.Get<Models::StreamModelPool::ModelNodeInstances>(instance.instance);
        Util::Array<Models::NodeBits>& bits = Models::modelPool->modelInstanceAllocator.Get<Models::StreamModelPool::ModelNodeBits>(instance.instance);
        if (hasPending[i])
        {
            uint objectId = Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ObjectId>(instance.instance);

            // copy matrix pending matrix
            Math::mat4 transform = pending[i];
            hasPending[i] = false;

            // transform the box
            instanceBoxes[instance.instance] = modelBoxes[instance.model];
            instanceBoxes[instance.instance].affine_transform(transform);

            // update the actual transform
            transforms[instance.instance] = transform;

            // nodes are allocated breadth first, so just going through the list will guarantee the hierarchy is traversed in proper order
            SizeT j;
            for (j = 0; j < nodes.Size(); j++)
            {
                Models::ModelNode::Instance* node = nodes[j];

                //if (!node->active)
                //  continue;

                Math::mat4 parentTransform = transform;
                if (node->parent != nullptr && (node->parent->node->bits & HasTransformBit) == HasTransformBit)
                    parentTransform = reinterpret_cast<const TransformNode::Instance*>(node->parent)->modelTransform;

                if ((bits[j] & HasTransformBit) == HasTransformBit)
                {
                    TransformNode::Instance* tnode = reinterpret_cast<TransformNode::Instance*>(node);
                    tnode->modelTransform = tnode->transform.getmatrix() * parentTransform;
                    tnode->invModelTransform = inverse(tnode->modelTransform);
                    parentTransform = tnode->modelTransform;
                    tnode->objectId = objectId;

                }
                if ((bits[j] & HasStateBit) == HasStateBit)
                {
                    ShaderStateNode::Instance* snode = reinterpret_cast<ShaderStateNode::Instance*>(node);

                    // copy bounding box from parent, then transform
                    snode->boundingBox = node->node->boundingBox;
                    snode->boundingBox.affine_transform(snode->modelTransform);
                }
                transform = parentTransform;
            }
        }

        // calculate view vector to calculate LOD
        Math::vec4 viewVector = cameraTransform.position - transforms[instance.instance].position;
        float viewDistance = length(viewVector);
        float textureLod = viewDistance - 38.5f;

        // nodes are allocated breadth first, so just going through the list will guarantee the hierarchy is traversed in proper order
        SizeT j;
        for (j = 0; j < nodes.Size(); j++)
        {
            Models::ModelNode::Instance* node = nodes[j];
            if ((bits[j] & HasTransformBit) == HasTransformBit)
            {
                TransformNode::Instance* tnode = reinterpret_cast<TransformNode::Instance*>(node);
                TransformNode* pnode = reinterpret_cast<TransformNode*>(tnode->node);

                // perform LOD testing
                if (pnode->useLodDistances)
                {
                    if (viewDistance >= pnode->minDistance && viewDistance < pnode->maxDistance)
                        tnode->active = true;
                    else
                        tnode->active = false;
                    tnode->lodFactor = (viewDistance - (pnode->minDistance + 1.5f)) / (pnode->maxDistance - (pnode->minDistance + 1.5f));
                }
                else
                    tnode->lodFactor = 0.0f;
            }

            if ((bits[j] & HasStateBit) == HasStateBit)
            {
                ShaderStateNode::Instance* snode = reinterpret_cast<ShaderStateNode::Instance*>(node);
                ShaderStateNode* pnode = reinterpret_cast<ShaderStateNode*>(node->node);
                if (!snode->active)
                    continue;

                snode->SetDirty(true);
                pnode->SetMaxLOD(textureLod);
            }
        }
    }
    */
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::OnRenderDebug(uint32_t flags)
{
    //const Util::Array<ModelInstanceId>& instances = modelContextAllocator.GetArray<Model_InstanceId>();    
    //Util::Array<Math::bbox>& instanceBoxes = Models::modelPool->modelInstanceAllocator.GetArray<StreamModelPool::InstanceBoundingBox>();
    //const Util::Array<Math::mat4>& transforms = Models::modelPool->modelInstanceAllocator.GetArray<StreamModelPool::InstanceTransform>();
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
