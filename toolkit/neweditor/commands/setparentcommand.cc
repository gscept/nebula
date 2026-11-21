//------------------------------------------------------------------------------
//  @file setparentcommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/setparentcommand.h"
#include "game/component.h"
#include "game/world.h"

namespace Editor
{

__ImplementClass(Editor::SetParentCommand, 'SEPC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
SetParentCommand::Name() const
{
    return this->newParent == Game::Entity::Invalid() ? "Clear parent" : "Set parent";
}

//------------------------------------------------------------------------------
/**
    The assignment runs through the scene on the first run, because that is where the cycle check
    and the local values come from. A redo puts the stored transform back instead, because the
    parent chain the values were computed against is not the same one anymore.
*/
bool
SetParentCommand::Execute()
{
    Game::World* world = this->scene->GetAuthoringWorld();
    const Game::ComponentId transform = Game::GetComponentId<Game::HTransform>();

    if (!this->initialized)
    {
        this->oldHasTransform = world->HasComponent(this->child, transform);
        if (this->oldHasTransform)
            this->oldTransform = world->GetComponent<Game::HTransform>(this->child);

        this->oldInstance = this->scene->GetEditable(this->child).prefabInstance;

        const bool result = this->newParent == Game::Entity::Invalid()
            ? this->scene->ClearParent(this->child)
            : this->scene->SetParent(this->child, this->newParent);
        if (!result)
            return false;

        this->newTransform = world->GetComponent<Game::HTransform>(this->child);
        this->newInstance = this->scene->GetEditable(this->child).prefabInstance;
        this->initialized = true;
        return true;
    }

    if (!world->HasComponent(this->child, transform))
        this->scene->AddComponent(this->child, transform, &this->newTransform);
    else
        this->scene->SetComponent(this->child, transform, &this->newTransform);

    // A redo puts the membership back as it was, because the tree the membership was worked out
    // against is not the same one anymore
    if (this->newInstance != this->oldInstance)
    {
        if (this->newInstance != InvalidIndex)
            this->scene->JoinInstance(this->child, this->newInstance);
        else
            this->scene->LeaveInstance(this->child);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
SetParentCommand::Unexecute()
{
    const Game::ComponentId transform = Game::GetComponentId<Game::HTransform>();

    if (!this->oldHasTransform)
        this->scene->RemoveComponent(this->child, transform);
    else
        this->scene->SetComponent(this->child, transform, &this->oldTransform);

    // The membership goes back the other way, because the tree the child was put into is what
    // decided it
    if (this->newInstance != this->oldInstance)
    {
        if (this->oldInstance == InvalidIndex)
            this->scene->LeaveInstance(this->child);
        else
            this->scene->JoinInstance(this->child, this->oldInstance);
    }
    return true;
}

} // namespace Editor
