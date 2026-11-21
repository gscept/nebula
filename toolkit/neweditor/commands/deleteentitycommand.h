#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::DeleteEntityCommand

    Deletes an authored entity. The record and the component values are captured before the entity
    is removed, because the instance is what goes, and a redo has to put the entity back as it was.

    Only the instance is taken out. The entity id is held by the command until it is destroyed,
    which is what keeps the entity references in the other commands valid while the entity is not in
    the world.

    The children keep their parent field, so the command also remembers which entities were children
    of the one being deleted, and the transforms they had. A child whose parent is gone resolves to
    its world values, which is not the transform it had, so the undo puts back what was captured
    here.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "basegamefeature/components/basegamefeature.h"
#include "core/refcounted.h"
#include "core/ptr.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"
#include "game/componentid.h"
#include "game/entity.h"
#include "util/array.h"
#include "util/blob.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
class DeleteEntityCommand : public EditCommand
{
    __DeclareClass(DeleteEntityCommand)
public:
    virtual ~DeleteEntityCommand();

    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    Game::Entity entity;

private:
    /// the state the entity is put back with
    Editable record;
    Util::Array<Game::ComponentId> components;
    Util::Array<Util::Blob> values;
    /// the entities that were children of this one
    Util::Array<Game::Entity> children;
    /// the transforms the children had. A child whose parent is gone resolves to its world values,
    /// so the
    /// transform put back has to be the one captured here rather than the one the child has by then
    Util::Array<Game::HTransform> childTransforms;
    /// the instance the entity was part of, and the marker the delete leaves for a record the file
    /// has.
    /// An entity the instance added has no record, so its delete leaves no marker
    IndexT instanceIndex = InvalidIndex;
    RemovedRecord marker;
    bool wroteMarker = false;
    bool initialized;
};

} // namespace Editor
