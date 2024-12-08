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

    /// increment to disallow picking temporarily. Remember to decrement when done.
    int pauseCounter = 0;

};

} // namespace Tools
