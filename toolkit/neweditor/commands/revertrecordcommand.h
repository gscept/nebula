#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::RevertRecordCommand

    Puts a record the instance marked removed back into the scene. The entity id is allocated on the
    first run and kept, which is what keeps the entity references in the other commands valid across
    an undo, the guid is the identity a redo puts back, and the marker is what the undo puts back.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "core/refcounted.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"
#include "game/entity.h"
#include "util/guid.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
class RevertRecordCommand : public EditCommand
{
    __DeclareClass(RevertRecordCommand)
public:
    virtual ~RevertRecordCommand();

    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    IndexT instanceIndex = InvalidIndex;
    Util::Guid record;

private:
    /// the identity the entity is put back with on a redo
    Util::Guid entityGuid;
    /// the entity the record gets. The id is held by the command until it is destroyed
    Game::Entity entity = Game::Entity::Invalid();
    /// the marker the undo puts back
    RemovedRecord marker;
    bool initialized = false;
};

} // namespace Editor
