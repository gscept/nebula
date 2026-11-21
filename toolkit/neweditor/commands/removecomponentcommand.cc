//------------------------------------------------------------------------------
//  @file removecomponentcommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/removecomponentcommand.h"
#include "game/world.h"
#include "memdb/attributeregistry.h"

namespace Editor
{

__ImplementClass(Editor::RemoveComponentCommand, 'RMCC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
RemoveComponentCommand::Name() const
{
    return "Remove component";
}

//------------------------------------------------------------------------------
/**
    The data has to be read before the component is gone, because the row the component lives in is
    released with it.
*/
bool
RemoveComponentCommand::Execute()
{
    // The owner and the three transform components are the fixed columns of every entity table.
    // Taking one
    // off moves the instance to a table that has no column for it, which is what the engine asserts
    // against, so these components cannot come off an entity
    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();
    if (this->component == ownerId || this->scene->IsTransformComponent(this->component))
        return false;

    if (!this->scene->GetAuthoringWorld()->HasComponent(this->entity, this->component))
        return false;

    if (!this->initialized)
    {
        const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(this->component);
        if (typeSize > 0)
            this->value.Set(this->scene->GetComponentData(this->entity, this->component), typeSize);

        const PrefabOverride& state = this->scene->FindPrefabOverride(this->entity, this->component);
        if (state.component == this->component)
        {
            this->hadState = true;
            this->oldState = state;
        }

        Util::Blob base;
        this->inChain = this->scene->GetPrefabBaseValue(this->entity, this->component, base);

        this->initialized = true;
    }

    this->scene->RemoveComponent(this->entity, this->component);

    // A component the chain gives the entity is removed against the chain, which is what the marker
    // says
    // and what the next save writes back. A component the instance added is not in the chain, so
    // there is
    // nothing left to say about it once the entity does not have it
    if (this->inChain)
    {
        PrefabOverride removed;
        removed.component = this->component;
        removed.componentRemoved = true;
        this->scene->SetPrefabOverride(this->entity, removed);
    }
    else
    {
        this->scene->ClearPrefabOverride(this->entity, this->component);
    }

    return true;
}

//------------------------------------------------------------------------------
/**
    The value and the state go back in that order, which is the same as a revert: putting back the
    remembered state decides what the entity had, because writing the value would otherwise
    recompute it.
*/
bool
RemoveComponentCommand::Unexecute()
{
    this->scene->AddComponent(this->entity, this->component, this->value.IsValid() ? this->value.GetPtr() : nullptr);

    if (this->hadState)
        this->scene->SetPrefabOverride(this->entity, this->oldState);
    else
        this->scene->ClearPrefabOverride(this->entity, this->component);

    return true;
}

} // namespace Editor
