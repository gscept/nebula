#pragma once
//------------------------------------------------------------------------------
/**
    @file   selectioncontext.h

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/singleton.h"
#include "editor/editor.h"

namespace Editor
{
class Camera;
}

namespace Edit
{
struct CMDSetSelection;
}

namespace Tools
{

class SelectionContext
{
    __DeclareSingleton(SelectionContext)
public:
    /// create singleton
    static void Create();
    /// destroy singleton
    static void Destroy();


    /// return the current selection
    static Util::Array<Editor::Entity> const& Selection();

    static void PerformPicking(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera);

    /// return the entity currently hovered, or invalid if none
    static Editor::Entity const& Hovered();

    /// set the entity currently hovered. This is a transient per-frame state and is not undoable.
    static void SetHovered(Editor::Entity entity);

    /// clear the hovered entity. Called at the start of every frame.
    static void ClearHovered();

    static void ValidateSelection();

    /// Pause picking momentarily. Remember to PickingContext::Unpause when done.
    static void Pause();
    /// Unpause picking.
    static void Unpause();
    /// Check if currently paused
    static bool IsPaused();

    /// returns the selected entity that is directly under the mouse, or invalid if none is under.
    static Editor::Entity GetSelectedEntityUnderMouse(
        Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera
    );

private:
    SelectionContext();
    ~SelectionContext();

    friend Edit::CMDSetSelection;

    Util::Array<Editor::Entity> selection;

    /// the entity currently hovered in the outline. transient per-frame state, not undoable.
    Editor::Entity hovered = Editor::Entity::Invalid();

    /// increment to disallow picking temporarily. Remember to decrement when done.
    int pauseCounter = 0;

};

} // namespace Tools
