#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::AddComponentCommand

    Adds a component to an authored entity. A component the entity already has is not an action, so
    the command says it did not run and the command manager does not remember it.

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
class AddComponentCommand : public EditCommand
{
    __DeclareClass(AddComponentCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    Game::Entity entity;
    Game::ComponentId component;
    /// the value the component is added with. Empty means the default value
    Util::Blob value;
};

} // namespace Editor
