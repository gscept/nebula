#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::DeleteCollectionCommand

    Deletes an authored collection. Deleting one does not delete what is in it, it moves the nested
    collections and the entities to the parent, so the command remembers the collection as it was
    and what the delete moved.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"
#include "game/entity.h"
#include "util/array.h"
#include "util/guid.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
class DeleteCollectionCommand : public EditCommand
{
    __DeclareClass(DeleteCollectionCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    Util::Guid collection;

private:
    /// the collection as it was, and the index it was taken out of, because the outline draws the
    /// collections in the order they are in the array
    Collection record;
    IndexT index;
    /// what the delete moved to the parent
    Util::Array<Util::Guid> childCollections;
    Util::Array<Game::Entity> entities;
    bool initialized;
};

} // namespace Editor
