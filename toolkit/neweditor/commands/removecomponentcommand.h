#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::RemoveComponentCommand

    Removes a component from an authored entity. The value is taken before the component is removed,
    because a redo has to add the component back with the data it had.

    The owner and the three transform components are the fixed columns of every entity table, so
    they are not removable, and the command refuses them.

    For an instance entity the removal is a change against its chain, and the marker is what says
    so: a component the chain gives is marked removed, and a component the instance added has
    nothing left to say once the entity does not have it. The state the entity had is kept for the
    same reason the value is.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"
#include "game/componentid.h"
#include "game/entity.h"
#include "util/blob.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
class RemoveComponentCommand : public EditCommand
{
    __DeclareClass(RemoveComponentCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    Game::Entity entity;
    Game::ComponentId component;

private:
    /// the data the component had before it was removed
    Util::Blob value;
    /// the override state the entity had, which is what the undo puts back
    bool hadState = false;
    PrefabOverride oldState;
    /// whether the chain has the component, which is what decides whether the removal is something
    /// the
    /// file has to be told about
    bool inChain = false;
    bool initialized;
};

} // namespace Editor
