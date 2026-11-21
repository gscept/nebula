#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::CreateCollectionCommand

    Creates an authored collection. The record is the collection as it was created and it is kept,
    so a redo puts the same collection back with the same identity.

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
class CreateCollectionCommand : public EditCommand
{
    __DeclareClass(CreateCollectionCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    /// the collection as it is created. The guid is generated on the first run and kept
    Collection record;
};

} // namespace Editor
