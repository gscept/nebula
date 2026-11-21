//------------------------------------------------------------------------------
//  @file selectioncontext.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/model/selectioncontext.h"
#include "neweditor/commands/setselectioncommand.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/api/shell.h"
#include "neweditor/model/scene.h"

namespace Editor
{

__ImplementClass(Editor::SelectionContext, 'SLCT', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::Setup(Shell* core, Scene* scene)
{
    this->core = core;
    this->scene = scene;
    this->manager = scene->GetCommandManager();
    this->selection.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::Discard()
{
    this->selection.Clear();
    this->manager = nullptr;
    this->scene = nullptr;
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Game::Entity>&
SelectionContext::GetSelection() const
{
    return this->selection;
}

//------------------------------------------------------------------------------
/**
*/
bool
SelectionContext::IsSelected(Game::Entity entity) const
{
    return this->selection.BinarySearchIndex(entity) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    A selection that is the same as the current one is not an action, so it does not become an undo
    entry.
*/
void
SelectionContext::SetSelection(const Util::Array<Game::Entity>& selection)
{
    Util::Array<Game::Entity> sorted = selection;
    sorted.Sort();

    if (sorted == this->selection)
        return;

    Ptr<SetSelectionCommand> command = SetSelectionCommand::Create();
    command->context = this;
    command->oldSelection = this->selection;
    command->newSelection = sorted;
    this->manager->Execute(command);
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::Toggle(Game::Entity entity)
{
    Util::Array<Game::Entity> selection = this->selection;

    const IndexT index = selection.BinarySearchIndex(entity);
    if (index != InvalidIndex)
        selection.EraseIndex(index);
    else
        selection.InsertSorted(entity);

    this->SetSelection(selection);
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::Clear()
{
    this->SetSelection(Util::Array<Game::Entity>());
}

//------------------------------------------------------------------------------
/**
    Entities that were deleted are not in the row list anymore, and an entity that was never
    authored was never in the selection to begin with.
*/
void
SelectionContext::Validate()
{
    if (this->selection.IsEmpty())
        return;

    Util::Array<Game::Entity> valid;
    for (IndexT i = 0; i < this->selection.Size(); i++)
    {
        if (this->scene->GetEditable(this->selection[i]).guid.IsValid() && this->scene->HasInstance(this->selection[i]))
            valid.Append(this->selection[i]);
    }

    if (valid.Size() != this->selection.Size())
        this->Apply(valid);
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionContext::Apply(const Util::Array<Game::Entity>& selection)
{
    this->selection = selection;
    // The panels read the selection when they draw, so there is nothing to tell them here
}

} // namespace Editor
