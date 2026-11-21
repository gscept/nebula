#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::RevertComponentCommand

    Puts one component of a prefab instance entity back to what its chain gives. The value and the
    override state the entity had are kept, so undoing the revert puts the entity back exactly as it
    was, added and removed marks included.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "core/refcounted.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"
#include "game/componentid.h"
#include "game/entity.h"
#include "util/blob.h"

namespace Editor
{

class RevertComponentCommand : public EditCommand
{
    __DeclareClass(RevertComponentCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    Game::Entity entity;
    Game::ComponentId component = Game::ComponentId::Invalid();

private:
    /// what the entity had before the revert
    bool initialized = false;
    bool hadComponent = false;
    Util::Blob oldValue;
    bool hadState = false;
    PrefabOverride oldState;
};

} // namespace Editor
