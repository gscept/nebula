//------------------------------------------------------------------------------
//  @file revertcomponentcommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/revertcomponentcommand.h"

namespace Editor
{

__ImplementClass(Editor::RevertComponentCommand, 'RVCC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
RevertComponentCommand::Name() const
{
    return "Revert to prefab";
}

//------------------------------------------------------------------------------
/**
    What the entity had is read before anything goes, because the revert takes the component out
    when the chain does not have it, and the row the value lived in goes with it.
*/
bool
RevertComponentCommand::Execute()
{
    if (this->component == Game::ComponentId::Invalid())
        return false;

    if (!this->initialized)
    {
        this->hadComponent = this->scene->HasComponent(this->entity, this->component);
        if (this->hadComponent)
        {
            const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(this->component);
            if (typeSize > 0)
                this->oldValue.Set(this->scene->GetComponentData(this->entity, this->component), typeSize);
        }

        const PrefabOverride& state = this->scene->FindPrefabOverride(this->entity, this->component);
        if (state.component == this->component)
        {
            this->hadState = true;
            this->oldState = state;
        }

        this->initialized = true;
    }

    return this->scene->RevertComponent(this->entity, this->component);
}

//------------------------------------------------------------------------------
/**
    The value and the state go back in that order: setting the value recomputes what is overridden,
    and putting back the remembered state decides it again.
*/
bool
RevertComponentCommand::Unexecute()
{
    if (this->hadComponent)
    {
        if (this->scene->HasComponent(this->entity, this->component))
            this->scene->SetComponent(this->entity, this->component, this->oldValue.GetPtr());
        else
            this->scene->AddComponent(this->entity, this->component,
                this->oldValue.IsValid() ? this->oldValue.GetPtr() : nullptr);
    }
    else if (this->scene->HasComponent(this->entity, this->component))
    {
        // The chain did not have the component, so the revert took an added one away
        this->scene->RemoveComponent(this->entity, this->component);
    }

    if (this->hadState)
        this->scene->SetPrefabOverride(this->entity, this->oldState);
    else
        this->scene->ClearPrefabOverride(this->entity, this->component);

    return true;
}

} // namespace Editor
