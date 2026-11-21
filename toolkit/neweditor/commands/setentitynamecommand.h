#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::SetEntityNameCommand

    Renames an authored entity. The name the entity had before is taken on the first run, so a redo
    puts the same name back.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"
#include "game/entity.h"
#include "util/string.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
class SetEntityNameCommand : public EditCommand
{
    __DeclareClass(SetEntityNameCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    Game::Entity entity;
    Util::String newName;

private:
    /// the name the entity had before the command ran
    Util::String oldName;
    bool initialized;
};

} // namespace Editor
