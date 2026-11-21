#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::SetCollectionParentCommand

    Moves an authored collection under another one. The parent the collection had before is taken on
    the first run, so a redo puts the collection back where it was.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"
#include "util/guid.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
class SetCollectionParentCommand : public EditCommand
{
    __DeclareClass(SetCollectionParentCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    Util::Guid collection;
    /// an invalid parent makes the collection a root collection
    Util::Guid newParent;

private:
    /// the parent the collection had before the command ran
    Util::Guid oldParent;
    bool initialized;
};

} // namespace Editor
