#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::SetCollectionNameCommand

    Renames an authored collection. The name the collection had before is taken on the first run, so
    a redo puts the same name back.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"
#include "util/guid.h"
#include "util/string.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
class SetCollectionNameCommand : public EditCommand
{
    __DeclareClass(SetCollectionNameCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    Util::Guid collection;
    Util::String newName;

private:
    /// the name the collection had before the command ran
    Util::String oldName;
    bool initialized;
};

} // namespace Editor
