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
#include "models/nodes/modelnode.h"
#include "materials/surfacepool.h"
#include "materials/materialtype.h"
#include "memory/arenaallocator.h"
#include "math/clipstatus.h"

namespace Models
{
    class ModelContext;
}

namespace Visibility
{

enum
{
    Observer_Matrix,
    Observer_EntityId,
    Observer_EntityType,
    Observer_ResultAllocator,
    Observer_Results,
    Observer_Dependency,
    Observer_DependencyMode,
    Observer_DrawList,
    Observer_DrawListAllocator
};

enum
{
    VisibilityResult_Flag,
    VisibilityResult_CtxId
};

enum
{
    ObservableAtom_Transform,
    ObservableAtom_Node,
    ObservableAtom_ContextEntity,
    ObservableAtom_Active
};

enum
{
    Observable_EntityId,
    Observable_EntityType,
    Observable_Atoms
};

enum DependencyMode
{
    DependencyMode_Total,       // if B depends on A, and A doesn't see B, B sees nothing either
    DependencyMode_Masked       // visibility of B is dependent on A for each result
};

class ObserverContext : public Graphics::GraphicsContext
{
    _DeclareContext();
public:

    /// setup entity
    static void Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType entityType);
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
    typedef Util::HashTable<Materials::MaterialType*,
        Util::HashTable<Models::ModelNode*,
        Util::Array<Models::ModelNode::DrawPacket*>>>
        VisibilityDrawList;

    /// get visibility draw list
    static const VisibilityDrawList* GetVisibilityDrawList(const Graphics::GraphicsEntityId id);

    static Jobs::JobPortId jobPort;
    static Jobs::JobSyncId jobInternalSync;
    static Jobs::JobSyncId jobInternalSync2;
    static Jobs::JobSyncId jobInternalSync3;
    static Jobs::JobSyncId jobHostSync;
    static Util::Queue<Jobs::JobId> runningJobs;

private:

    friend class ObservableContext;

    typedef Ids::IdAllocator<
        Math::ClipStatus::Type,             // visibility result
        Graphics::ContextEntityId           // model context id
    > VisibilityResultAllocator;

    typedef Ids::IdAllocator<
        Math::mat4,                     // transform of observer camera
        Graphics::GraphicsEntityId,         // entity id
        VisibilityEntityType,               // type of object so we know how to get the transform
        VisibilityResultAllocator,          // visibility lookup table
        Math::ClipStatus::Type*,            // array holding the visbility results array
        Graphics::GraphicsEntityId,         // dependency
        DependencyMode,                     // dependency mode
        VisibilityDrawList,                 // draw list
        Memory::ArenaAllocator<1024>        // memory allocator for draw commands
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
    _DeclareContext();
public:

    /// setup entity
    static void Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType entityType);

    /// create context
    static void Create();
private:

    friend class ObserverContext;
    friend class Models::ModelContext;

    // atom corresponds to a single visibility entry
    typedef Ids::IdAllocator<
        Math::mat4,
        Models::ModelNode::Instance*,
        Graphics::ContextEntityId,
        bool
    > ObservableAtomAllocator;
    static ObservableAtomAllocator observableAtomAllocator;

    // observable corresponds to a single entity
    typedef Ids::IdAllocator<
        Graphics::GraphicsEntityId,     // entity id
        VisibilityEntityType,           // type of object so we know how to get the transform
        Util::ArrayStack<Ids::Id32, 1>  // keep track of atoms
    > ObservableAllocator;

    static ObservableAllocator observableAllocator;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
    /// move instance
    static void OnInstanceMoved(uint32_t toIndex, uint32_t fromIndex);
    /// update model context id in visiblity results allocators.
    static void UpdateModelContextId(Graphics::GraphicsEntityId id, Graphics::ContextEntityId modelCid);
};

} // namespace Visibility
