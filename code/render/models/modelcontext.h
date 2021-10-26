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
#include "coregraphics/resourcetable.h"
#include "materials/materialserver.h"
#include "model.h"
#include "nodes/modelnode.h"

namespace Jobs
{
struct JobFuncContext;
struct JobSyncId;
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
    , NodeInstance_AlwaysVisible = N_BIT(3)     // Should always resolve to being visible by visibility
    , NodeInstance_Visible = N_BIT(4)           // Set to true if any observer sees it
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

    /// set the transform for a model
    static void SetTransform(const Graphics::GraphicsEntityId id, const Math::mat4& transform);
    /// get the transform for a model
    static Math::mat4 GetTransform(const Graphics::GraphicsEntityId id);
    /// get the transform for a model
    static Math::mat4 GetTransform(const Graphics::ContextEntityId id);

    /// runs before frame is updated
    static void UpdateTransforms(const Graphics::FrameContext& ctx);
    /// runs after BeginFrame
    static void UpdateConstants(const Graphics::FrameContext& ctx);
#ifndef PUBLIC_DEBUG    
    /// debug rendering
    static void OnRenderDebug(uint32_t flags);
#endif

    /// get model
    static const Models::ModelId GetModel(const Graphics::ContextEntityId id);
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
        IndexT particleConstantsIndex;
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
            Util::Array<Materials::SurfaceId> nodeSurfaces;
            Util::Array<NodeInstanceState> nodeStates;
            Util::Array<Materials::MaterialType*> nodeMaterialTypes;
            Util::Array<Models::ModelNode*> nodes;
            Util::Array<std::function<void()>> nodeModelCallbacks;
            Util::Array<Util::Tuple<uint32, uint32>> nodeDrawModifiers;

            Util::Array<void*> nodeSpecialData;
#if NEBULA_GRAPHICS_DEBUG
            Util::Array<Util::StringAtom> nodeNames;
#endif
        } renderable;

    };

    /// Get model node instance states
    static const NodeInstanceRange& GetModelRenderableRange(const Graphics::GraphicsEntityId id);
    /// Get array to all model node states
    static const Util::Array<NodeInstanceState>& GetModelRenderableStates();
    /// Get array to all model node instace bounding boxes
    static const Util::Array<Math::bbox>& GetModelRenderableBoundingBoxes();
    /// Get array to all model node instance flags
    static const Util::Array<NodeInstanceFlags>& GetModelRenderableFlags();
    /// Get node renderable context
    static const ModelInstance::Renderable& GetModelRenderables();
    /// Get node transformable context
    static const ModelInstance::Transformable& GetModelTransformables();

private:
    friend class VisibilityContext;
    friend void ModelRenderableUpdateJob(const Jobs::JobFuncContext& ctx);
    friend void ModelTransformUpdateJob(const Jobs::JobFuncContext& ctx);

    static ModelInstance nodeInstances;

    enum
    {
        Model_Id,
        Model_NodeInstanceRoots,
        Model_NodeInstanceTransform,
        Model_NodeInstanceStates,
        Model_Transform,
        Model_Dirty
    };
    typedef Ids::IdAllocator<
        ModelId,
        Util::Array<uint32>,
        NodeInstanceRange,
        NodeInstanceRange,
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

    if (rid != ModelId::Invalid()) // decrement model resource
        Models::DestroyModel(rid);
    rid = ModelId::Invalid();

    modelContextAllocator.Dealloc(id.id);
}

} // namespace Models
