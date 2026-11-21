#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::SetComponentCommand

    Sets one component value on an authored entity. The value the component had before is taken on
    the first run and kept, so a redo puts the same value back.

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
class SetComponentCommand : public EditCommand
{
    __DeclareClass(SetComponentCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    Game::Entity entity;
    Game::ComponentId component;
    Util::Blob newValue;

private:
    /// the value the component had before the command ran
    Util::Blob oldValue;
    bool initialized;
};

} // namespace Editor
