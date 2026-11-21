#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::SetParentCommand

    Makes an authored entity a child of another, or clears its parent when the new parent is
    invalid. Both transforms are stored, because the local values are computed from the parent chain
    and a redo cannot recompute them from the state it is putting back.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "basegamefeature/components/basegamefeature.h"
#include "core/refcounted.h"
#include "core/ptr.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"
#include "game/entity.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
class SetParentCommand : public EditCommand
{
    __DeclareClass(SetParentCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    Game::Entity child;
    /// an invalid parent clears the parent instead of setting one
    Game::Entity newParent;

private:
    /// the transform the child had before the command ran. When the child did not have the
    /// component, the
    /// command added it, and putting the child back means taking it away again
    Game::HTransform oldTransform;
    Game::HTransform newTransform;
    bool oldHasTransform;
    /// the instance the child was in before and after the command. Membership follows the tree, so
    /// an
    /// entity the level created under an instance entity joins it, and an addition put outside
    /// leaves it
    IndexT oldInstance = InvalidIndex;
    IndexT newInstance = InvalidIndex;
    bool initialized;
};

} // namespace Editor
