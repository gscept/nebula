#pragma once
//------------------------------------------------------------------------------
/**
    A model context bind a ModelInstance to a model, which allows it to be rendered using an .n3 file.

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "core/singleton.h"
#include "resources/resourceid.h"
#include "materials/materialserver.h"
#include "model.h"
#include "nodes/modelnode.h"

namespace Jobs
{
struct JobFuncContext;
};

namespace Visibility
{
void VisibilitySortJob(const Jobs::JobFuncContext& ctx);
};

namespace Models
{

enum NodeInstanceFlags
{
    NodeInstance_Active = N_BIT(1)              // If set, node is active to render
    , NodeInstance_LodActive = N_BIT(2)         // If set, the node's LOD is active
    , NodeInstance_Visible = N_BIT(3)           // Updated by visibility
    , NodeInstance_AlwaysVisible = N_BIT(4)     // Should always resolve to being visible by visibility
};

class ModelContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:
    /// constructor
    ModelContext();
    /// destructor
    virtual ~ModelContext();

    /// create context
    static void Create();

    /// setup
    static void Setup(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name, const Util::StringAtom& tag, std::function<void()> finishedCallback);

    /// change model for existing entity
    static void ChangeModel(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name, const Util::StringAtom& tag, std::function<void()> finishedCallback);
    /// get model
    static const Models::ModelId GetModel(const Graphics::GraphicsEntityId id);
    /// get model instance
    static const Models::ModelInstanceId GetModelInstance(const Graphics::GraphicsEntityId id);

    /// set the transform for a model
    static void SetTransform(const Graphics::GraphicsEntityId id, const Math::mat4& transform);
    /// get the transform for a model
    static Math::mat4 GetTransform(const Graphics::GraphicsEntityId id);
    /// get the transform for a model
    static Math::mat4 GetTransform(const Graphics::ContextEntityId id);
    /// get the bounding box
    static Math::bbox GetBoundingBox(const Graphics::GraphicsEntityId id);

    /// get model node instances
    static const Util::Array<Models::ModelNode::Instance*>& GetModelNodeInstances(const Graphics::GraphicsEntityId id);
    /// get model node types
    static const Util::Array<Models::NodeType>& GetModelNodeTypes(const Graphics::GraphicsEntityId id);

    /// runs before frame is updated
    static void UpdateTransforms(const Graphics::FrameContext& ctx);
#ifndef PUBLIC_DEBUG    
    /// debug rendering
    static void OnRenderDebug(uint32_t flags);
#endif

    /// get model
    static const Models::ModelId GetModel(const Graphics::ContextEntityId id);
    /// get model instance
    static const Models::ModelInstanceId GetModelInstance(const Graphics::ContextEntityId id);
    /// get model node instances
    static const Util::Array<Models::ModelNode::Instance*>& GetModelNodeInstances(const Graphics::ContextEntityId id);
    /// get model node instances
    static const Util::Array<Models::NodeType>& GetModelNodeTypes(const Graphics::ContextEntityId id);

    struct NodeInstanceState
    {
        CoreGraphics::ResourceTableId resourceTable;
        Materials::SurfaceInstanceId surfaceInstance;
        Util::FixedArray<uint32_t> resourceTableOffsets;
        IndexT objectConstantsIndex;
        IndexT instancingConstantsIndex;
        IndexT skinningConstantsIndex;
    };

    struct NodeInstanceRange
    {
        SizeT offset, size;
    };

    struct ModelInstance
    {
        /// Transforms are only used by the model context to traverse and propagate the transform hierarchy
        struct Transformable
        {
            Util::Array<Math::mat4> origTransforms;
            Util::Array<Math::mat4> nodeTransforms;
            Util::Array<uint32> nodeParents;
        } transformable;

        /// The bounding boxes are used by visibility and the states by rendering
        struct Renderable
        {
            Util::Array<Math::bbox> origBoundingBoxes;
            Util::Array<Math::bbox> nodeBoundingBoxes;
            Util::Array<Util::Tuple<float, float>> nodeLodDistances;
            Util::Array<float> nodeLods;
            Util::Array<uint32> nodeTransformIndex;
            Util::Array<uint64> nodeSortId;
            Util::Array<NodeInstanceFlags> nodeFlags;
            Util::Array<Materials::SurfaceResourceId> nodeSurfaceResources;
            Util::Array<NodeInstanceState> nodeStates;
            Util::Array<Materials::MaterialType*> nodeMaterialTypes;
            Util::Array<Models::ModelNode*> nodes;
        } renderable;
    };

    /// Get model node instance states
    static const NodeInstanceRange& GetModelNodeInstanceStateRange(const Graphics::GraphicsEntityId id);
    /// Get array to all model node states
    static const Util::Array<NodeInstanceState>& GetModelNodeInstanceStates();
    /// Get array to all model node instace bounding boxes
    static const Util::Array<Math::bbox>& GetModelNodeInstanceBoundingBoxes();
    /// Get array to all model node instance flags
    static const Util::Array<NodeInstanceFlags>& GetModelNodeInstanceFlags();
    /// Get node renderable context
    static ModelInstance::Renderable& GetModelNodeInstanceRenderables();

private:
    friend class VisibilityContext;
    friend void ModelBoundingBoxUpdateJob(const Jobs::JobFuncContext& ctx);
    friend void ModelTransformUpdateJob(const Jobs::JobFuncContext& ctx);

    static ModelInstance nodeInstances;

    enum
    {
        Model_Id,
        Model_NodeInstanceTransform,
        Model_NodeInstanceStates,
        Model_InstanceId,
        Model_Transform,
        Model_Dirty
    };
    typedef Ids::IdAllocator<
        ModelId,
        NodeInstanceRange,
        NodeInstanceRange,
        ModelInstanceId,
        Math::mat4,         // pending transforms
        bool                // transform is dirty
    > ModelContextAllocator;
    static ModelContextAllocator modelContextAllocator;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
    
    //friend void Visibility::VisibilitySortJob(const Jobs::JobFuncContext& ctx);

    static Jobs::JobSyncId jobInternalSync;
    static Jobs::JobSyncId jobHostSync;
};

//------------------------------------------------------------------------------
/**
*/
inline Graphics::ContextEntityId
ModelContext::Alloc()
{
    return modelContextAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelContext::Dealloc(Graphics::ContextEntityId id)
{
    // clean up old stuff, but don't deallocate entity
    ModelId& rid = modelContextAllocator.Get<Model_Id>(id.id);
    ModelInstanceId& mdl = modelContextAllocator.Get<Model_InstanceId>(id.id);

    if (mdl != ModelInstanceId::Invalid()) // actually deallocate current instance
        Models::DestroyModelInstance(mdl);
    if (rid != ModelId::Invalid()) // decrement model resource
        Models::DestroyModel(rid);
    mdl = ModelInstanceId::Invalid();
    rid = ModelId::Invalid();

    modelContextAllocator.Dealloc(id.id);
}

} // namespace Models
