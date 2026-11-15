//------------------------------------------------------------------------------
//  @file transformhierarchy.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "transformhierarchy.h"
#include "game/world.h"
#include "memdb/database.h"
#include "memdb/table.h"
#include "math/mat4.h"

namespace Game
{
__ImplementClass(Game::TransformHierarchy, 'Thie', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
void
TransformHierarchy::Initialize(Game::World* world)
{
    this->world = world;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformHierarchy::EnsureInstance(Game::Entity entity, bool hasLocalTransform)
{
    uint32_t instance = this->GetInstance(entity);
    if (instance != InvalidInstance)
    {
        if (hasLocalTransform)
        {
            this->data.Set<HasLocalTransform>(instance, true);
        }
        return instance;
    }

    instance = this->data.Alloc();
    if (entity.index >= (uint64_t)this->entityToInstance.Size())
    {
        this->entityToInstance.Resize((SizeT)entity.index + 1, InvalidInstance);
    }
    this->entityToInstance[entity.index] = instance;

    this->data.Set<OwnerEntity>(instance, entity);
    this->data.Set<Parent>(instance, InvalidInstance);
    this->data.Set<FirstChild>(instance, InvalidInstance);
    this->data.Set<NextSibling>(instance, InvalidInstance);
    this->data.Set<PreviousSibling>(instance, InvalidInstance);
    this->data.Set<HasLocalTransform>(instance, hasLocalTransform);

    this->entityInHierarchy.SetBit(entity.index);
    return instance;
}

//------------------------------------------------------------------------------
/**
    The instance of an entity, or InvalidInstance when the entity is not part of the hierarchy.
*/
uint32_t
TransformHierarchy::GetInstance(Game::Entity entity) const
{
    if (entity.index >= (uint64_t)this->entityToInstance.Size())
    {
        return InvalidInstance;
    }

    uint32_t const instance = this->entityToInstance[entity.index];
    if (instance == InvalidInstance || instance >= this->data.Size())
    {
        return InvalidInstance;
    }

    if (this->data.ConstGet<OwnerEntity>(instance) != entity)
    {
        return InvalidInstance;
    }
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
bool
TransformHierarchy::IsInHierarchy(Game::Entity entity) const
{
    return this->entityInHierarchy.IsSet(entity.index);
}

//------------------------------------------------------------------------------
/**
*/
bool
TransformHierarchy::HasChildren(uint32_t instance) const
{
    return this->data.ConstGet<FirstChild>(instance) != InvalidInstance;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
TransformHierarchy::GetOwner(uint32_t instance) const
{
    return this->data.ConstGet<OwnerEntity>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformHierarchy::GetParent(uint32_t instance) const
{
    return this->data.ConstGet<Parent>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformHierarchy::GetFirstChild(uint32_t instance) const
{
    return this->data.ConstGet<FirstChild>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformHierarchy::GetNextSibling(uint32_t instance) const
{
    return this->data.ConstGet<NextSibling>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformHierarchy::GetPreviousSibling(uint32_t instance) const
{
    return this->data.ConstGet<PreviousSibling>(instance);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TransformHierarchy::Size() const
{
    return (SizeT)this->data.Size();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TransformHierarchy::NumPending() const
{
    return this->pending.Size();
}

//------------------------------------------------------------------------------
/**
*/
TransformHierarchy::ComponentPointers
TransformHierarchy::GetComponents(Game::Entity entity, bool hasLocalTransform) const
{
    Game::EntityMapping const mapping = this->world->GetEntityMapping(entity);
    MemDb::TableId const tableId = mapping.table;
    MemDb::Table& table = this->world->GetDatabase()->GetTable(tableId);
    uint16_t const partition = mapping.instance.partition;
    uint32_t const row = mapping.instance.index;

    ComponentPointers components;
    components.local = nullptr;
    if (hasLocalTransform)
    {
        IndexT const cached = this->localColumn.FindIndex(tableId);
        MemDb::ColumnIndex column = MemDb::ColumnIndex::Invalid();
        if (cached != InvalidIndex)
        {
            column = this->localColumn.ValueAtIndex(cached);
        }
        else
        {
            column = table.GetAttributeIndex(Game::GetComponentId<Game::HTransform>());
            if (column != MemDb::ColumnIndex::Invalid())
            {
                this->localColumn.Add(tableId, column);
            }
        }

        // A component can be queued without being in its table yet, because the add commands
        // are executed after the frame pipeline.
        if (column != MemDb::ColumnIndex::Invalid())
        {
            components.local = (Game::HTransform*)table.GetBuffer(partition, column);
            components.local += row;
        }
    }

    components.position = (Game::Position*)table.GetBuffer(partition, Game::Position::Traits::fixed_column_index);
    components.orientation = (Game::Orientation*)table.GetBuffer(partition, Game::Orientation::Traits::fixed_column_index);
    components.scale = (Game::Scale*)table.GetBuffer(partition, Game::Scale::Traits::fixed_column_index);

    components.position += row;
    components.orientation += row;
    components.scale += row;
    return components;
}

//------------------------------------------------------------------------------
/**
    Take an instance out of the links. The components are not touched here, which makes this half
    of a detach. Remove use it for the entity it takes out of the hierarchy.
*/
void
TransformHierarchy::Unlink(uint32_t instance)
{
    uint32_t const parentInstance = this->data.ConstGet<Parent>(instance);
    if (parentInstance == InvalidInstance)
    {
        return;
    }

    uint32_t const previous = this->data.ConstGet<PreviousSibling>(instance);
    uint32_t const next = this->data.ConstGet<NextSibling>(instance);
    if (this->data.Get<FirstChild>(parentInstance) == instance)
    {
        this->data.Get<FirstChild>(parentInstance) = next;
    }
    if (previous != InvalidInstance)
    {
        this->data.Get<NextSibling>(previous) = next;
    }
    if (next != InvalidInstance)
    {
        this->data.Get<PreviousSibling>(next) = previous;
    }

    this->data.Get<Parent>(instance) = InvalidInstance;

    // the instance is no longer part of a child list, so it should not name the list it was in
    this->data.Get<PreviousSibling>(instance) = InvalidInstance;
    this->data.Get<NextSibling>(instance) = InvalidInstance;
}

//------------------------------------------------------------------------------
/**
    Link an instance under a parent instance. The link is refused when it would create a cycle.
*/
bool
TransformHierarchy::LinkUnderParent(uint32_t instance, uint32_t parentInstance)
{
    if (instance == InvalidInstance || parentInstance == InvalidInstance || instance == parentInstance)
    {
        return false;
    }

    uint32_t ancestor = parentInstance;
    while (ancestor != InvalidInstance)
    {
        if (ancestor == instance)
        {
            n_warning("Cycle detected in transform hierarchy; detaching entity %llu\n",
                (unsigned long long)(uint64_t)this->data.ConstGet<OwnerEntity>(instance));
            return false;
        }
        ancestor = this->data.Get<Parent>(ancestor);
    }

    this->Unlink(instance);

    uint32_t const firstChild = this->data.Get<FirstChild>(parentInstance);
    this->data.Set<NextSibling>(instance, firstChild);
    this->data.Set<PreviousSibling>(instance, InvalidInstance);
    if (firstChild != InvalidInstance)
    {
        this->data.Set<PreviousSibling>(firstChild, instance);
    }
    this->data.Set<FirstChild>(parentInstance, instance);

    this->data.Set<Parent>(instance, parentInstance);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
TransformHierarchy::Detach(uint32_t instance, ComponentPointers const& components)
{
    if (components.local == nullptr)
    {
        // the component is not in a table yet, so it cannot be written. The component is the authority, and it still
        // names its parent, so there is nothing to detach here
        return;
    }

    this->Unlink(instance);

    components.local->parent = Game::Entity::Invalid();
    components.local->localPosition = *components.position;
    components.local->localOrientation = *components.orientation;
    components.local->localScale = *components.scale;
    this->world->MarkAsModified(this->data.ConstGet<OwnerEntity>(instance));
}

//------------------------------------------------------------------------------
/**
    Make the local transform be the world transform the components hold. This is the expensive part of a world
    write, because it needs the inverse of the parent, and it is only done for an instance which has a local
    transform of its own.
*/
void
TransformHierarchy::DeriveLocal(uint32_t instance, ComponentPointers const& components)
{
    if (components.local == nullptr)
    {
        return;
    }

    uint32_t const parentInstance = this->data.ConstGet<Parent>(instance);
    if (parentInstance == InvalidInstance)
    {
        // a root: its local transform is its world transform
        components.local->localPosition = *components.position;
        components.local->localOrientation = *components.orientation;
        components.local->localScale = *components.scale;
        this->world->MarkAsModified(this->data.ConstGet<OwnerEntity>(instance));
        return;
    }

    Game::Entity const parentOwner = this->data.ConstGet<OwnerEntity>(parentInstance);
    if (!this->world->IsValid(parentOwner) || !this->world->HasInstance(parentOwner))
    {
        return;
    }

    ComponentPointers const parent = this->GetComponents(parentOwner, false);

    Math::quat const inverseParentRotation = Math::inverse(*parent.orientation);

    Math::vec3 const offset = Math::vec3(
        components.position->x - parent.position->x,
        components.position->y - parent.position->y,
        components.position->z - parent.position->z);

    Math::vec3 const rotated = Math::rotate(inverseParentRotation, offset);

    Math::vec3 localPosition = rotated;
    Math::vec3 localScale = *components.scale;

    // a scale of zero has no inverse, which is the case for a collider which was collapsed rather than scaled
    if (parent.scale->x != 0.0f)
    {
        localPosition.x /= parent.scale->x;
        localScale.x /= parent.scale->x;
    }
    if (parent.scale->y != 0.0f)
    {
        localPosition.y /= parent.scale->y;
        localScale.y /= parent.scale->y;
    }
    if (parent.scale->z != 0.0f)
    {
        localPosition.z /= parent.scale->z;
        localScale.z /= parent.scale->z;
    }

    components.local->localPosition = localPosition;

    components.local->localOrientation = inverseParentRotation * *components.orientation;
    components.local->localScale = localScale;
    this->world->MarkAsModified(this->data.ConstGet<OwnerEntity>(instance));
}

//------------------------------------------------------------------------------
/**
    Compute one instance from what its components hold, write the world transform, and put its children in the walk
    queue.
*/
void
TransformHierarchy::Compute(uint32_t instance)
{
    Game::Entity const owner = this->data.ConstGet<OwnerEntity>(instance);
    if (!this->world->IsValid(owner))
    {
        // The entity is gone. An instance which only exists to be a parent has no component whose decay could remove
        // it, so an instance can outlive its entity. Its children become roots with the world they have, and the
        // instance itself is taken out of the hierarchy by the flush after the walk, because the erasure moves
        // another instance into the slot it frees while the walk is using the slots
        uint32_t child = this->data.ConstGet<FirstChild>(instance);
        while (child != InvalidInstance)
        {
            uint32_t const sibling = this->data.Get<NextSibling>(child);
            Game::Entity const childOwner = this->data.Get<OwnerEntity>(child);
            if (this->world->IsValid(childOwner) && this->world->HasInstance(childOwner))
            {
                ComponentPointers const components = this->GetComponents(childOwner, true);
                this->Detach(child, components);
            }
            this->data.Get<Parent>(child) = InvalidInstance;
            this->data.Get<PreviousSibling>(child) = InvalidInstance;
            child = sibling;
        }
        this->stale.Append(instance);
        return;
    }

    if (!this->world->HasInstance(owner))
    {
        this->pending.Append(owner);
        return;
    }

    bool const hasLocal = this->data.ConstGet<HasLocalTransform>(instance);
    ComponentPointers const components = this->GetComponents(owner, hasLocal);
    if (hasLocal && components.local == nullptr)
    {
        this->pending.Append(owner);
        return;
    }

    if (hasLocal)
    {
        // the component is the authority for the parent, so an instance is put under the parent its component names
        // when the link does not already say so. A parent which is not in the hierarchy yet becomes an instance
        // here, because an instance is an entity which has a local transform or which is the parent of one
        uint32_t componentParent = InvalidInstance;
        Game::Entity const namedParent = components.local->parent;
        if (namedParent != Game::Entity::Invalid())
        {
            if (namedParent.world == this->world->GetWorldId() && this->world->IsValid(namedParent))
            {
                componentParent = this->EnsureInstance(namedParent, false);
            }
            else
            {
                uint32_t const staleParent = this->GetInstance(namedParent);
                if (staleParent != InvalidInstance)
                {
                    this->stale.Append(staleParent);
                }
                this->Detach(instance, components);
            }
        }

        if (componentParent != this->data.ConstGet<Parent>(instance))
        {
            if (!this->LinkUnderParent(instance, componentParent))
            {
                this->Unlink(instance);
            }
        }
    }

    Math::vec3 position = *components.position;
    Math::quat orientation = *components.orientation;
    Math::vec3 scale = *components.scale;

    uint32_t const parentInstance = this->data.ConstGet<Parent>(instance);
    if (parentInstance != InvalidInstance)
    {
        Game::Entity const parentOwner = this->data.ConstGet<OwnerEntity>(parentInstance);
        if (!this->world->IsValid(parentOwner))
        {
            this->Detach(instance, components);
            this->stale.Append(parentInstance);
        }
        else if (!this->world->HasInstance(parentOwner))
        {
            // the parent is not in its table yet, so its components cannot be read. The instance is computed again
            // when they are, and with it the instances below it
            this->pending.Append(owner);
            return;
        }
        else
        {
            ComponentPointers const parent = this->GetComponents(parentOwner, false);

            Math::vec3 const scaledPoint = Math::vec3(
                parent.scale->x * components.local->localPosition.x,
                parent.scale->y * components.local->localPosition.y,
                parent.scale->z * components.local->localPosition.z);
            Math::vec3 const rotatedPoint = Math::rotate(*parent.orientation, scaledPoint);

            position = Math::vec3(parent.position->x + rotatedPoint.x, parent.position->y + rotatedPoint.y, parent.position->z + rotatedPoint.z);

            orientation = *parent.orientation * components.local->localOrientation;
            scale = Math::vec3(
                parent.scale->x * components.local->localScale.x,
                parent.scale->y * components.local->localScale.y,
                parent.scale->z * components.local->localScale.z);
        }
    }

    if (*components.position != Game::Position(position) ||
        *components.orientation != Game::Orientation(orientation) ||
        *components.scale != Game::Scale(scale))
    {
        *components.position = Game::Position(position);
        *components.orientation = Game::Orientation(orientation);
        *components.scale = Game::Scale(scale);
        this->world->MarkAsModified(owner);
    }

    uint32_t child = this->data.ConstGet<FirstChild>(instance);
    while (child != InvalidInstance)
    {
        this->stack.Append(child);
        child = this->data.Get<NextSibling>(child);
    }
}

//------------------------------------------------------------------------------
/**
    Walk the instances below an instance, computing each of them.
*/
void
TransformHierarchy::Walk(uint32_t instance)
{
    this->stack.Clear();
    this->stack.Append(instance);

    while (this->stack.Size() > 0)
    {
        uint32_t const walked = this->stack.PopBack();
        this->Compute(walked);
    }
}

//------------------------------------------------------------------------------
/**
    Take out the instances which a walk found to name an entity that is deleted.
*/
void
TransformHierarchy::FlushStale()
{
    if (this->stale.Size() == 0)
    {
        return;
    }

    for (SizeT i = 0; i < this->stale.Size(); i++)
    {
        // Two children of the same stale parent put the same instance in the queue, and the first flush moves another
        // instance into the slot it frees. An instance is only stale when the entity it names is gone, which is what
        // makes the second one a no-op
        Game::Entity const owner = this->data.ConstGet<OwnerEntity>(this->stale[i]);
        if (!this->world->IsValid(owner))
        {
            this->RemoveInstance(this->stale[i]);
        }
    }
    this->stale.Clear();
}

//------------------------------------------------------------------------------
/**
    Add an entity to the hierarchy with the parent its component names, and compute it.
*/
uint32_t
TransformHierarchy::Add(Game::Entity entity, Game::Entity parent)
{
    uint32_t const instance = this->EnsureInstance(entity, true);

    if (parent != Game::Entity::Invalid() && parent.world == this->world->GetWorldId() && this->world->IsValid(parent))
    {
        uint32_t const parentInstance = this->EnsureInstance(parent, false);
        this->LinkUnderParent(instance, parentInstance);
    }

    this->Walk(instance);
    this->FlushStale();
    return instance;
}

//------------------------------------------------------------------------------
/**
    Set the parent of an instance and compute it
*/
void
TransformHierarchy::SetParent(uint32_t instance, uint32_t parentInstance)
{
    if (instance == InvalidInstance)
    {
        return;
    }

    if (this->data.ConstGet<Parent>(instance) != parentInstance)
    {
        if (!this->LinkUnderParent(instance, parentInstance))
        {
            // the parent cannot be linked, which is the case when there is no parent, or when the link would close a
            // cycle. The component is the authority, so the instance becomes a root
            this->Unlink(instance);
        }
    }

    this->Walk(instance);
    this->FlushStale();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformHierarchy::SetParent(Game::Entity entity, Game::Entity parent)
{
    uint32_t const instance = this->GetInstance(entity);
    if (instance == InvalidInstance)
    {
        return;
    }

    uint32_t parentInstance = InvalidInstance;
    if (parent != Game::Entity::Invalid())
    {
        if (parent.world == this->world->GetWorldId() && this->world->IsValid(parent))
        {
            parentInstance = this->EnsureInstance(parent, false);
        }
    }

    this->SetParent(instance, parentInstance);
}

//------------------------------------------------------------------------------
/**
    Set the local transform of an instance, write it to the component which owns it, and propagate.
*/
void
TransformHierarchy::SetLocalTransform(uint32_t instance, Math::mat4 const& value)
{
    if (instance == InvalidInstance)
    {
        return;
    }

    Game::Entity const owner = this->data.ConstGet<OwnerEntity>(instance);
    if (!this->world->IsValid(owner) || !this->world->HasInstance(owner))
    {
        return;
    }

    ComponentPointers const components = this->GetComponents(owner, true);
    if (components.local == nullptr)
    {
        return;
    }

    Math::vec3 localPosition;
    Math::quat localOrientation;
    Math::vec3 localScale;
    Math::decompose(value, localScale, localOrientation, localPosition);

    components.local->localPosition = localPosition;
    components.local->localOrientation = localOrientation;
    components.local->localScale = localScale;
    this->world->MarkAsModified(owner);

    this->Walk(instance);
    this->FlushStale();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformHierarchy::SetLocalTransform(Game::Entity entity, Math::mat4 const& value)
{
    uint32_t const instance = this->GetInstance(entity);
    if (instance != InvalidInstance)
    {
        this->SetLocalTransform(instance, value);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TransformHierarchy::SetLocalTransform(uint32_t instance)
{
    if (instance == InvalidInstance)
    {
        return;
    }

    this->Walk(instance);
    this->FlushStale();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformHierarchy::SetLocalTransform(Game::Entity entity)
{
    uint32_t const instance = this->GetInstance(entity);
    if (instance != InvalidInstance)
    {
        this->SetLocalTransform(instance);
    }
}

//------------------------------------------------------------------------------
/**
    Set the world transform of an instance, derive the local transform it implies, and propagate. An instance which
    has no local transform of its own keeps the world transform as it was written.
*/
void
TransformHierarchy::SetWorldTransform(uint32_t instance, Math::mat4 const& value)
{
    if (instance == InvalidInstance)
    {
        return;
    }

    Game::Entity const owner = this->data.ConstGet<OwnerEntity>(instance);
    if (!this->world->IsValid(owner) || !this->world->HasInstance(owner))
    {
        return;
    }

    ComponentPointers const components = this->GetComponents(owner, this->data.ConstGet<HasLocalTransform>(instance));

    Math::vec3 worldPosition;
    Math::quat worldOrientation;
    Math::vec3 worldScale;
    Math::decompose(value, worldScale, worldOrientation, worldPosition);

    if (*components.position != Game::Position(worldPosition) ||
        *components.orientation != Game::Orientation(worldOrientation) ||
        *components.scale != Game::Scale(worldScale))
    {
        *components.position = Game::Position(worldPosition);
        *components.orientation = Game::Orientation(worldOrientation);
        *components.scale = Game::Scale(worldScale);
        this->world->MarkAsModified(owner);
    }

    // the world which was written is what the components hold now, so the walk composes from the local this derived,
    // and the two agree
    this->DeriveLocal(instance, components);
    this->Walk(instance);
    this->FlushStale();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformHierarchy::SetWorldTransform(Game::Entity entity, Math::mat4 const& value)
{
    uint32_t const instance = this->GetInstance(entity);
    if (instance != InvalidInstance)
    {
        this->SetWorldTransform(instance, value);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TransformHierarchy::SetWorldTransform(uint32_t instance)
{
    if (instance == InvalidInstance)
    {
        return;
    }

    Game::Entity const owner = this->data.ConstGet<OwnerEntity>(instance);
    if (!this->world->IsValid(owner) || !this->world->HasInstance(owner))
    {
        return;
    }

    ComponentPointers const components = this->GetComponents(owner, this->data.ConstGet<HasLocalTransform>(instance));
    if (this->data.ConstGet<HasLocalTransform>(instance))
    {
        this->DeriveLocal(instance, components);
    }

    this->Walk(instance);
    this->FlushStale();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformHierarchy::SetWorldTransform(Game::Entity entity)
{
    uint32_t const instance = this->GetInstance(entity);
    if (instance != InvalidInstance)
    {
        this->SetWorldTransform(instance);
    }
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
TransformHierarchy::GetLocalTransform(uint32_t instance) const
{
    if (instance == InvalidInstance || instance >= this->data.Size())
    {
        return Math::mat4::identity;
    }

    Game::Entity const owner = this->data.ConstGet<OwnerEntity>(instance);
    if (!this->world->IsValid(owner) || !this->world->HasInstance(owner))
    {
        return Math::mat4::identity;
    }

    ComponentPointers const components = this->GetComponents(owner, true);
    if (components.local == nullptr)
    {
        return Math::mat4::identity;
    }

    return Math::trs(components.local->localPosition, components.local->localOrientation, components.local->localScale);
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
TransformHierarchy::GetLocalTransform(Game::Entity entity) const
{
    return this->GetLocalTransform(this->GetInstance(entity));
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
TransformHierarchy::GetWorldTransform(uint32_t instance) const
{
    if (instance == InvalidInstance || instance >= this->data.Size())
    {
        return Math::mat4::identity;
    }

    Game::Entity const owner = this->data.ConstGet<OwnerEntity>(instance);
    if (!this->world->IsValid(owner) || !this->world->HasInstance(owner))
    {
        return Math::mat4::identity;
    }

    ComponentPointers const components = this->GetComponents(owner, false);
    return Math::trs(*components.position, *components.orientation, *components.scale);
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
TransformHierarchy::GetWorldTransform(Game::Entity entity) const
{
    return this->GetWorldTransform(this->GetInstance(entity));
}

//------------------------------------------------------------------------------
/**
    Remove an entity from the hierarchy. Its children become roots with the world transform they have.
*/
void
TransformHierarchy::Remove(Game::Entity entity)
{
    this->RemoveInstance(this->GetInstance(entity));
    this->entityInHierarchy.ClearBit(entity.index);
}

//------------------------------------------------------------------------------
/**
    Remove an instance. Pop'n'swaps instances.
*/
void
TransformHierarchy::RemoveInstance(uint32_t instance)
{
    if (instance == InvalidInstance || instance >= this->data.Size())
    {
        return;
    }

    uint32_t const parentInstance = this->data.ConstGet<Parent>(instance);
    if (parentInstance != InvalidInstance)
    {
        uint32_t const previous = this->data.ConstGet<PreviousSibling>(instance);
        uint32_t const next = this->data.ConstGet<NextSibling>(instance);
        if (this->data.Get<FirstChild>(parentInstance) == instance)
        {
            this->data.Get<FirstChild>(parentInstance) = next;
        }
        if (previous != InvalidInstance)
        {
            this->data.Get<NextSibling>(previous) = next;
        }
        if (next != InvalidInstance)
        {
            this->data.Get<PreviousSibling>(next) = previous;
        }
    }

    // the children become roots, and their local transform becomes the world transform they have
    uint32_t child = this->data.ConstGet<FirstChild>(instance);
    while (child != InvalidInstance)
    {
        uint32_t const sibling = this->data.Get<NextSibling>(child);
        Game::Entity const childOwner = this->data.Get<OwnerEntity>(child);
        if (this->world->IsValid(childOwner) && this->world->HasInstance(childOwner))
        {
            ComponentPointers const components = this->GetComponents(childOwner, true);
            this->Detach(child, components);
        }
        this->data.Get<Parent>(child) = InvalidInstance;
        this->data.Get<PreviousSibling>(child) = InvalidInstance;
        child = sibling;
    }

    Game::Entity const owner = this->data.ConstGet<OwnerEntity>(instance);
    if (owner.index < (uint64_t)this->entityToInstance.Size())
    {
        this->entityToInstance[owner.index] = InvalidInstance;
    }
    this->entityInHierarchy.ClearBit(owner.index);

    uint32_t const last = (uint32_t)this->data.Size() - 1;
    if (last != instance && last != InvalidInstance)
    {
        Game::Entity const moved = this->data.Get<OwnerEntity>(last);
        if (moved.index < (uint64_t)this->entityToInstance.Size())
        {
            this->entityToInstance[moved.index] = instance;
        }

        uint32_t const movedParent = this->data.ConstGet<Parent>(last);
        if (movedParent != InvalidInstance)
        {
            uint32_t const movedPrevious = this->data.ConstGet<PreviousSibling>(last);
            uint32_t const movedNext = this->data.ConstGet<NextSibling>(last);
            if (this->data.Get<FirstChild>(movedParent) == last)
            {
                this->data.Get<FirstChild>(movedParent) = instance;
            }
            if (movedPrevious != InvalidInstance)
            {
                this->data.Get<NextSibling>(movedPrevious) = instance;
            }
            if (movedNext != InvalidInstance)
            {
                this->data.Get<PreviousSibling>(movedNext) = instance;
            }
        }

        // the moved instance can also be a parent, and the instances below it name the slot it came from
        uint32_t movedChild = this->data.ConstGet<FirstChild>(last);
        while (movedChild != InvalidInstance)
        {
            uint32_t const movedSibling = this->data.Get<NextSibling>(movedChild);
            this->data.Get<Parent>(movedChild) = instance;
            movedChild = movedSibling;
        }
    }

    this->data.EraseIndexSwap(instance);
}

//------------------------------------------------------------------------------
/**
    Compute the instances which could not be computed when they were written.
*/
void
TransformHierarchy::ResolvePending()
{
    if (this->pending.Size() == 0)
    {
        return;
    }

    Util::Array<Game::Entity> snapshot = this->pending;
    this->pending.Clear();

    for (SizeT i = 0; i < snapshot.Size(); i++)
    {
        uint32_t const instance = this->GetInstance(snapshot[i]);
        if (instance != InvalidInstance)
        {
            this->Walk(instance);
        }
    }

    this->FlushStale();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformHierarchy::Clear()
{
    this->data.Clear();
    this->entityToInstance.Clear();
    this->entityInHierarchy.Reset();
    this->localColumn.Clear();
    this->stack.Clear();
    this->pending.Clear();
    this->stale.Clear();
}

//------------------------------------------------------------------------------
/**
    Component decay hook. Taking the local transform component away takes the entity out of the hierarchy, and its
    children become roots with the world transform they have.
*/
void
DecayHTransform(Game::World* world, Game::Entity entity, Game::HTransform* component)
{
    world->GetHierarchy()->Remove(entity);
}

} // namespace Game
//------------------------------------------------------------------------------
