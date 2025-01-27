#pragma once
//------------------------------------------------------------------------------
/**
    Contains two contexts, one for observers and one for observables

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "visibility.h"
#include "jobs/jobs.h"
#include "visibility/systems/visibilitysystem.h"
#include "models/model.h"
#include "models/nodes/shaderstatenode.h"
#include "materials/materialtemplates.h"
#include "memory/arenaallocator.h"
#include "math/clipstatus.h"
#include "coregraphics/mesh.h"

namespace Models
{
    class ModelContext;
}

namespace Visibility
{

enum
{
    Observer_Matrix,
    Observer_IsOrtho,
    Observer_EntityId,
    Observer_EntityType,
    Observer_ResultArray,
    Observer_Dependency,
    Observer_DependencyMode,
    Observer_DrawList,
    Observer_DrawListAllocator,
};

enum
{
    ObservableAtom_NodeInstanceRange,
    ObservableAtom_GraphicsEntityId,
    ObservableAtom_Transform,
    ObservableAtom_Instance,
    ObservableAtom_VisibilityEntityType,
    ObservableAtom_Active
};

enum
{
    Observable_EntityId,
    Observable_NumNodes
};

enum DependencyMode
{
    DependencyMode_Total,       // if B depends on A, and A doesn't see B, B sees nothing either
    DependencyMode_Masked       // visibility of B is dependent on A for each result
};

class ObserverContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:

    /// setup entity
    static void Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType entityType, bool isOrtho = false);
    /// setup a dependency between observers
    static void MakeDependency(const Graphics::GraphicsEntityId a, const Graphics::GraphicsEntityId b, const DependencyMode mode);

    /// run visibility testing
    static void RunVisibilityTests(const Graphics::FrameContext& ctx);
    /// runs before frame is updated
    static void GenerateDrawLists(const Graphics::FrameContext& ctx);

    /// create context
    static void Create();
    /// discard context
    static void Discard();

    /// create a box system
    static VisibilitySystem* CreateBoxSystem(const BoxSystemLoadInfo& info);
    /// create a portal system
    static VisibilitySystem* CreatePortalSystem(const PortalSystemLoadInfo& info);
    /// create octree system
    static VisibilitySystem* CreateOctreeSystem(const OctreeSystemLoadInfo& info);
    /// create quadtree system
    static VisibilitySystem* CreateQuadtreeSystem(const QuadtreeSystemLoadInfo& info);
    /// create brute force system
    static VisibilitySystem* CreateBruteforceSystem(const BruteforceSystemLoadInfo& info);

    /// wait for all visibility jobs
    static void WaitForVisibility(const Graphics::FrameContext& ctx);

#ifndef PUBLIC_BUILD
    /// render debug
    static void OnRenderDebug(uint32_t flags);
#endif

    typedef Ids::Id32 ModelAllocId;

    struct VisibilityModelCommand
    {
        uint32 offset;
        CoreGraphics::MeshId mesh;
        CoreGraphics::PrimitiveGroup primitiveGroup;
        Materials::MaterialId material;

#if NEBULA_GRAPHICS_DEBUG
        Util::StringAtom nodeName;
#endif
    };

    struct VisibilityDrawCommand
    {
        uint32 offset;
        uint32 numInstances;
        uint32 baseInstance;
    };

    struct VisibilityBatchCommand
    {
        uint32 packetOffset;
        uint32 numDrawPackets;
        Util::Array<VisibilityModelCommand, 256> models;
        Util::Array<VisibilityDrawCommand, 1024> draws;
    };

    struct VisibilityDrawList
    {
        Util::HashTable<const MaterialTemplates::Entry*, VisibilityBatchCommand> visibilityTable;
        Util::Array<Models::ShaderStateNode::DrawPacket*> drawPackets;
    };

    /// get visibility draw list
    static const VisibilityDrawList* GetVisibilityDrawList(const Graphics::GraphicsEntityId id);

    static Jobs::JobSyncId jobInternalSync;
    static Jobs::JobSyncId jobInternalSync2;
    static Jobs::JobSyncId jobInternalSync3;
    static Jobs::JobSyncId jobHostSync;
    static Util::Queue<Jobs::JobId> runningJobs;

private:

    friend class ObservableContext;
    friend struct ObservableGlobalState;
    typedef Util::Array<Math::ClipStatus::Type> VisibilityResultArray;

    typedef Ids::IdAllocator<
        Math::mat4                                 // transform of observer camera
        , bool                                     // observer is an orthogonal camera
        , Graphics::GraphicsEntityId               // entity id
        , VisibilityEntityType                     // type of object so we know how to get the transform
        , VisibilityResultArray                    // visibility lookup table
        , Graphics::GraphicsEntityId               // dependency
        , DependencyMode                           // dependency mode
        , VisibilityDrawList                       // draw list
        , Memory::ArenaAllocator<1024>             // memory allocator for draw commands
    > ObserverAllocator;
    static ObserverAllocator observerAllocator;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
    
    /// keep as ordinary array of pointers, no need to have them cache aligned
    static Util::Array<VisibilitySystem*> systems;
};

class ObservableContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:

    /// setup entity
    static void Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType entityType);

    /// create context
    static void Create();
private:

    friend class ObserverContext;
    friend class Models::ModelContext;

    // observable corresponds to a single entity
    typedef Ids::IdAllocator<
        Graphics::GraphicsEntityId, // The entity id that this component is bound to
        uint32                      // Node instance range
    > ObservableAllocator;

    static ObservableAllocator observableAllocator;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
};

} // namespace Visibility
