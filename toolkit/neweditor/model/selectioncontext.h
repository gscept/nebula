#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::SelectionContext

    The current selection. The selection is a sorted set of authored entities, which is what makes
    membership a binary search instead of a scan.

    The selection belongs to one scene, so a tab that is not showing keeps its own.

    Setting the selection goes through the command manager, which is what makes it undoable.
    Applying one does not, because a command has to be able to apply a selection without creating
    another command.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "game/entity.h"
#include "util/array.h"

namespace Editor
{

class Shell;
class EditCommandManager;
class Scene;
class SetSelectionCommand;

class SelectionContext : public Core::RefCounted
{
    __DeclareClass(SelectionContext)
public:
    /// the core the selection reports to and the scene it selects in. The manager it goes through
    /// is the
    /// scene's
    void Setup(Shell* core, Scene* scene);
    /// clear the selection
    void Discard();

    /// the current selection, sorted by entity
    const Util::Array<Game::Entity>& GetSelection() const;
    /// true if an entity is in the selection
    bool IsSelected(Game::Entity entity) const;

    /// set the selection through the command manager
    void SetSelection(const Util::Array<Game::Entity>& selection);
    /// add or remove one entity through the command manager
    void Toggle(Game::Entity entity);

    /// clear the selection
    void Clear();

    /// drop entities that are no longer authored. This is a correction and not an action, so it is
    /// not undoable
    void Validate();

private:
    friend class SetSelectionCommand;

    /// apply a selection without going through the command manager
    void Apply(const Util::Array<Game::Entity>& selection);

    Shell* core;
    Scene* scene;
    Ptr<EditCommandManager> manager;
    Util::Array<Game::Entity> selection;
};

} // namespace Editor
