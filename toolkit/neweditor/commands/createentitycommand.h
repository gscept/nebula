#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::CreateEntityCommand

    Creates an authored entity. The record is the identity the entity is created with and it is
    kept, so a redo puts the same entity back with the same identity.

    Only the instance is put in and taken out. The entity id is allocated on the first run and held
    by the command until it is destroyed, which is what keeps the entity references in the other
    commands valid across an undo.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
class CreateEntityCommand : public EditCommand
{
    __DeclareClass(CreateEntityCommand)
public:
    virtual ~CreateEntityCommand();

    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    /// the identity the entity is created with. The guid is generated on the first run and kept
    Editable record;
};

} // namespace Editor
