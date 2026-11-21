#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::PropertyCommand

    One managed property, changed. The command holds the entity's text before and the text it has
    after, and it applies one of them. The text is the entity as one node per property, which is the
    shape the level format writes, so an empty text is an entity with nothing on it and the same
    command covers an add, a field change and a removal.

    The command does not know what a managed property is or what its fields are. It hands the text
    to the scene, which hands it to the extension that produced it. That is what keeps the editor
    from needing reflection over data it cannot see, and it is why the text is what the undo stack
    holds.

    The path is the key inside the text the change is about, as the extension names it. A path with
    no field is the whole property. This is the only thing that says which part of the text changed,
    and it is what the override marker is keyed by.

    @see
    Editor::PropertyRecorder

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
#include "util/string.h"
#include "util/stringatom.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
class PropertyCommand : public EditCommand
{
    __DeclareClass(PropertyCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    Game::Entity entity;
    /// the extension the property belongs to, which is what the override marker is keyed by
    Util::StringAtom extension;
    /// the key inside the text the change is about, as the extension names it. A path with no field
    /// is the
    /// whole property, which is what an add, a removal and a whole-property revert report
    Util::String path;
    /// the entity's text before and after. An empty text is an entity with nothing on it, which is
    /// what a
    /// command holds when the property it changed was the only one the entity had
    Util::String oldText;
    Util::String newText;
    /// the keys the entity had authored before and after. The marker and the file have to agree, so
    /// an undo
    /// that only put the text back would leave the next save writing the wrong thing
    Util::Array<Util::String> oldFields;
    Util::Array<Util::String> newFields;
    /// true when the after text is what the prefab chain gives rather than what a widget wrote. The
    /// before and
    /// after are the same shape either way, so this is the only thing that tells a revert from a
    /// change
    bool isRevert = false;
};

} // namespace Editor
//------------------------------------------------------------------------------
