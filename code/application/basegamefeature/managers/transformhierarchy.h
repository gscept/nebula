#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::TransformHierarchy

    @brief The parent, child and sibling links of the entities in one world's hierarchy.

    @details The hierarchy is the index of which entity is below which. The transforms are not stored here:
    the local transform is the Game::HTransform component and the world transform is Game::Position,
    Game::Orientation and Game::Scale, which is what every consumer reads through the query buffers.

    An instance is an entity which has a local transform, or an entity which is the parent of one, because a
    world transform written on a parent has to reach the entities below it.

    A write which goes through Game::World::SetComponent is computed at the time it happens, and it propagates
    to the instances below it, so a transform read after a write is the transform which was written. Propagation
    goes one way only, towards the leaves.

    IsInHierarchy() is the test a transform writer makes before it does anything. It is one bit in a dense
    array, keyed by entity.index, which is what keeps the common case - an entity which is not part of a
    hierarchy - on a single cache line. The bit says that the entity had an instance when the bit was set, so
    an instance which outlived its entity can still answer true for a reused entity id. The instance lookup
    checks the generation and answers false for that, which is the only case where a write pays the lookup.

    A component which has been queued but not executed yet is not in a table, so it cannot be computed. Such an
    instance is put in the pending set, and ResolvePending() computes it when its components, and the components
    of its parent, are in a table.

    @see Game::World

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/scale.h"
#include "game/entity.h"
#include "math/mat4.h"
#include "memdb/tableid.h"
#include "util/array.h"
#include "util/arrayallocator.h"
#include "util/bitarray.h"
#include "util/dictionary.h"

namespace Game
{
class World;

class TransformHierarchy : public Core::RefCounted
{
    __DeclareClass(TransformHierarchy)
public:
    /// an instance which is not part of the hierarchy
    static constexpr uint32_t InvalidInstance = 0xFFFFFFFF;

    /// the world this hierarchy belongs to. Set by the world which owns it
    void Initialize(Game::World* world);

    /// Set the local transform of an instance, compute the world transform it implies and propagate to the
    /// instances below it. The no-value form takes what the components already hold as the input, which is what
    /// a writer which wrote through a query buffer needs.
    void SetLocalTransform(uint32_t instance, Math::mat4 const& value);
    void SetLocalTransform(Game::Entity entity, Math::mat4 const& value);
    void SetLocalTransform(uint32_t instance);
    void SetLocalTransform(Game::Entity entity);

    /// Set the world transform of an instance, which derives the local transform it implies through the inverse
    /// of the parent, and propagate to the instances below it.
    void SetWorldTransform(uint32_t instance, Math::mat4 const& value);
    void SetWorldTransform(Game::Entity entity, Math::mat4 const& value);
    void SetWorldTransform(uint32_t instance);
    void SetWorldTransform(Game::Entity entity);

    /// the transforms of an instance, read from the components which own them
    Math::mat4 GetLocalTransform(uint32_t instance) const;
    Math::mat4 GetLocalTransform(Game::Entity entity) const;
    Math::mat4 GetWorldTransform(uint32_t instance) const;
    Math::mat4 GetWorldTransform(Game::Entity entity) const;

    /// Update relationships.
    void SetParent(Game::Entity entity, Game::Entity parent);
    void SetParent(uint32_t instance, uint32_t parentInstance);

    /// Get parent of instance
    uint32_t GetParent(uint32_t instance) const;

    /// Get owner of instance
    Game::Entity GetOwner(uint32_t instance) const;

    /// Get first child of instance
    uint32_t GetFirstChild(uint32_t instance) const;

    /// Get the first sibling of this instance
    uint32_t GetNextSibling(uint32_t instance) const;

    /// Get the previous sibling of this instance
    uint32_t GetPreviousSibling(uint32_t instance) const;

    /// an instance which has children is one the writer has to walk into
    bool HasChildren(uint32_t instance) const;

    /// the instance of an entity, or InvalidInstance when the entity is not part of the hierarchy
    uint32_t GetInstance(Game::Entity entity) const;

    /// the fast test for a transform writer, one bit in a dense array keyed by entity.index
    bool IsInHierarchy(Game::Entity entity) const;

    /// add an entity to the hierarchy with the parent its component names, and compute it
    uint32_t Add(Game::Entity entity, Game::Entity parent);

    /// remove an entity from the hierarchy. Its children become roots with the world transform they have
    void Remove(Game::Entity entity);

    /// remove an instance, patching the links which name the slot the swap-erase moves into it
    void RemoveInstance(uint32_t instance);

    /// compute the instances which could not be computed when they were written
    void ResolvePending();

    /// number of hierarchy instances, and number of instances waiting for their components
    SizeT Size() const;
    SizeT NumPending() const;

    /// clear the hierarchy
    void Clear();

private:
    /// the component pointers of an instance. A pointer is null when the component is not in the table the
    /// entity is in, which is the case for a component which has been queued but not executed yet
    struct ComponentPointers
    {
        Game::HTransform* local;
        Game::Position* position;
        Game::Orientation* orientation;
        Game::Scale* scale;
    };

    /// link an instance under a parent instance. False when the link would close a cycle
    bool LinkUnderParent(uint32_t instance, uint32_t parentInstance);

    /// take an instance out of the links, without touching its components
    void Unlink(uint32_t instance);

    /// make the local transform of an instance be the world transform it currently has, and detach it
    void Detach(uint32_t instance, ComponentPointers const& components);

    /// derive the local transform from the world transform the components hold
    void DeriveLocal(uint32_t instance, ComponentPointers const& components);

    /// compute one instance from what its components hold and put its children in the walk queue
    void Compute(uint32_t instance);

    /// walk the instances below an instance, computing each of them
    void Walk(uint32_t instance);

    /// take out the instances which were found to name an entity that is gone during a walk
    void FlushStale();

    /// the instance of an entity, creating it when it is not part of the hierarchy yet
    uint32_t EnsureInstance(Game::Entity entity, bool hasLocalTransform);

    /// the component pointers of an entity. The transform columns have fixed indices in an entity table, the
    /// local transform column does not, and is null when the table does not have it
    ComponentPointers GetComponents(Game::Entity entity, bool hasLocalTransform) const;

    Game::World* world;

    /// the members of the instance data
    enum Member
    {
        OwnerEntity = 0,
        Parent = 1,
        FirstChild = 2,
        NextSibling = 3,
        PreviousSibling = 4,
        HasLocalTransform = 5,
    };
    
    Util::ArrayAllocator<
        Game::Entity,
        uint32_t,
        uint32_t,
        uint32_t,
        uint32_t,
        bool> data;

    // entity.index is the array index in this map
    Util::Array<uint32_t> entityToInstance;

    // one bit per entity, which is what makes the test for an entity which is not in a hierarchy cheap
    Util::BitArray entityInHierarchy;

    // the cache is filled by a const lookup, which is what makes it mutable
    mutable Util::Dictionary<MemDb::TableId, MemDb::ColumnIndex> localColumn;
    Util::Array<uint32_t> stack;
    Util::Array<Game::Entity> pending;

    // instances which named an entity that is gone. They are taken out of the hierarchy after the walk, because
    // the erasure moves another instance into the slot they free while the walk is using the slots
    Util::Array<uint32_t> stale;
};

//------------------------------------------------------------------------------
/**
    Component decay hook for Game::HTransform. Taking the component away takes the entity out of the hierarchy,
    and its children become roots with the world transform they have.
*/
void DecayHTransform(Game::World* world, Game::Entity entity, Game::HTransform* component);

} // namespace Game
//------------------------------------------------------------------------------
