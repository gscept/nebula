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
#include "resources/resourceserver.h"
#include "coregraphics/resourcetable.h"
#include "materials/shaderconfigserver.h"
#include "model.h"
#include "nodes/modelnode.h"

namespace Jobs
{
    struct JobFuncContext;
    struct JobSyncId;
};

namespace Materials
{
    struct MaterialId;
};

namespace CoreGraphics
{
    struct MeshId;
};

namespace Models
{

enum class NodeInstanceFlags
{
    NodeInstance_Active = N_BIT(1)              // If set, node is active to render
    , NodeInstance_LodActive = N_BIT(2)         // If set, the node's LOD is active
    , NodeInstance_AlwaysVisible = N_BIT(3)     // Should always resolve to being visible by visibility
    , NodeInstance_Visible = N_BIT(4)           // Set to true if any observer sees it
    , NodeInstance_Moved = N_BIT(5)
};
__ImplementEnumBitOperators(NodeInstanceFlags);

class ModelContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:
    struct MaterialInstanceContext;

    /// constructor
    ModelContext();
    /// destructor
    virtual ~ModelContext();

    /// create context
    static void Create();

    /// setup
    static void Setup(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name, const Util::StringAtom& tag, std::function<void()> finishedCallback);
    /// Setup without a model resource
    static void Setup(
        const Graphics::GraphicsEntityId id
        , const Math::mat4 transform
        , const Math::bbox& boundingBox
        , const Materials::MaterialId material
        , const CoreGraphics::MeshId mesh
        , const IndexT primitiveGroup
#if NEBULA_GRAPHICS_DEBUG
        , const Util::String debugName = ""
#endif
    );

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

    /// Get node index based on name
    static IndexT GetNodeIndex(const Graphics::GraphicsEntityId id, const Util::StringAtom& name);
    /// Setup material instance context
    static MaterialInstanceContext& SetupMaterialInstanceContext(const Graphics::GraphicsEntityId id, const IndexT nodeIndex, const CoreGraphics::BatchGroup::Code batch);
    /// Setup material instance context
    static MaterialInstanceContext& SetupMaterialInstanceContext(const Graphics::GraphicsEntityId id, const CoreGraphics::BatchGroup::Code batch);
    /// Allocate constant memory for instance constants in this frame
    static CoreGraphics::ConstantBufferOffset AllocateInstanceConstants(const Graphics::GraphicsEntityId id, const IndexT nodeIndex, const Materials::BatchIndex batch);
    /// Allocate constant memory for instance constants in this frame
    static CoreGraphics::ConstantBufferOffset AllocateInstanceConstants(const Graphics::GraphicsEntityId id, const Materials::BatchIndex batch);

    /// Set all nodes in the model to always be visible
    static void SetAlwaysVisible(const Graphics::GraphicsEntityId id);

    /// runs before frame is updated
    static void UpdateTransforms(const Graphics::FrameContext& ctx);
    /// Wait for thread work to finish
    static void WaitForWork(const Graphics::FrameContext& ctx);
#ifndef PUBLIC_DEBUG    
    /// debug rendering
    static void OnRenderDebug(uint32_t flags);
#endif

    /// get model
    static const Models::ModelId GetModel(const Graphics::ContextEntityId id);
     
    struct NodeInstanceState
    {
        Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;
        Materials::MaterialInstanceId materialInstance;
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
            Util::PinnedArray<0xFFFF, Math::mat4> origTransforms;
            Util::PinnedArray<0xFFFF, Math::mat4> nodeTransforms;
            Util::PinnedArray<0xFFFF, uint32> nodeParents;
        } transformable;

        /// The bounding boxes are used by visibility and the states by rendering
        struct Renderable
        {
            Util::PinnedArray<0xFFFF, Math::bbox> origBoundingBoxes;
            Util::PinnedArray<0xFFFF, Math::bbox> nodeBoundingBoxes;
            Util::PinnedArray<0xFFFF, Util::Tuple<float, float>> nodeLodDistances;
            Util::PinnedArray<0xFFFF, float> nodeLods;
            Util::PinnedArray<0xFFFF, float> textureLods;
            Util::PinnedArray<0xFFFF, uint32> nodeTransformIndex;
            Util::PinnedArray<0xFFFF, uint64> nodeSortId;
            Util::PinnedArray<0xFFFF, NodeInstanceFlags> nodeFlags;
            Util::PinnedArray<0xFFFF, Materials::MaterialId> nodeMaterials;
            Util::PinnedArray<0xFFFF, Materials::ShaderConfig*> nodeShaderConfigs;
            Util::PinnedArray<0xFFFF, NodeInstanceState> nodeStates;
            Util::PinnedArray<0xFFFF, Models::NodeType> nodeTypes;
            Util::PinnedArray<0xFFFF, Models::ModelNode*> nodes;
            Util::PinnedArray<0xFFFF, CoreGraphics::MeshId> nodeMeshes;
            Util::PinnedArray<0xFFFF, CoreGraphics::PrimitiveGroup> nodePrimitiveGroup;
            Util::PinnedArray<0xFFFF, Util::Tuple<uint32, uint32>> nodeDrawModifiers;

            Util::PinnedArray<0xFFFF, void*> nodeSpecialData;
#if NEBULA_GRAPHICS_DEBUG
            Util::PinnedArray<0xFFFF, Util::StringAtom> nodeNames;
#endif
        } renderable;

    };

    struct MaterialInstanceContext
    {
        Materials::BatchIndex batch;
        SizeT constantBufferSize;
    };

    /// Get model node instance states
    static const NodeInstanceRange& GetModelRenderableRange(const Graphics::GraphicsEntityId id);
    /// Get model node instance transformables
    static const NodeInstanceRange& GetModelTransformableRange(const Graphics::GraphicsEntityId id);
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
    /// Get if model is loaded
    static bool IsLoaded(const Graphics::GraphicsEntityId id);

    static Threading::AtomicCounter ConstantsUpdateCounter;
    static Threading::AtomicCounter TransformsUpdateCounter;

private:
    friend class VisibilityContext;

    static ModelInstance NodeInstances;
    static Memory::SCAllocator TransformInstanceAllocator, RenderInstanceAllocator;

    enum
    {
        Model_Id,
        Model_NodeInstanceRoots,
        Model_NodeInstanceTransform,
        Model_NodeInstanceStates,
        Model_NodeLookup,
        Model_Transform,
        Model_Dirty
    };
    typedef Ids::IdAllocator<
        Resources::ResourceId,
        Util::Array<uint32>,
        NodeInstanceRange,
        NodeInstanceRange,
        Util::Dictionary<Util::StringAtom, IndexT>,
        Math::mat4,         // pending transforms
        bool                // transform is dirty
    > ModelContextAllocator;
    static ModelContextAllocator modelContextAllocator;

    static Util::Dictionary<Models::ModelNode*, MaterialInstanceContext> materialInstanceContexts;

    static Threading::Event completionEvent;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
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
    Resources::ResourceId rid = modelContextAllocator.Get<Model_Id>(id.id);

    if (rid != Resources::InvalidResourceId) // decrement model resource
        Resources::DiscardResource(rid);
    rid = Resources::InvalidResourceId;

    modelContextAllocator.Get<Model_NodeInstanceRoots>(id.id).Clear();
    modelContextAllocator.Get<Model_NodeLookup>(id.id).Clear();
    TransformInstanceAllocator.Dealloc(modelContextAllocator.Get<Model_NodeInstanceTransform>(id.id).allocation);
    RenderInstanceAllocator.Dealloc(modelContextAllocator.Get<Model_NodeInstanceStates>(id.id).allocation);

    modelContextAllocator.Dealloc(id.id);
}

} // namespace Models
