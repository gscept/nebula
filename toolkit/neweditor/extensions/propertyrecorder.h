#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::PropertyRecorder

    The editor's undo half of a level extension's data. A feature that holds data the editor cannot
    read reports its changes here, and the command is built from them. The recorder does not know
    what a property is or what its fields are. It resolves the extension by name and uses the hooks
    the extension registered.

    Only what a feature reports is recorded, because a runtime change is not an authoring change and
    does not belong on an undo stack.

    A report comes from the draw, so it is queued and run after the frame. There is one entry per
    entity, so a drag that reports every frame becomes one command.

    @see
    Game::LevelExtension

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "neweditor/api/shell.h"
#include "game/entity.h"
#include "util/array.h"
#include "util/string.h"
#include "util/stringatom.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
    One change a feature reported for an entity, before it has been run.
*/
struct PendingPropertyChange
{
    Game::Entity entity;
    /// the extension the change belongs to
    Util::StringAtom extension;
    /// the key inside the extension's text, named the way the extension names it. With no key it is
    /// the whole
    /// extension
    Util::String path;
    /// the entity's text before and after, which is what the command applies and what the level
    /// format writes
    Util::String oldText;
    Util::String newText;
    /// the keys the entity had authored before and after. Only a linked entity has a mark, because
    /// an entity
    /// that is its own authored everything
    Util::Array<Util::String> oldFields;
    Util::Array<Util::String> newFields;
    /// true when the new text is what the prefab chain gives rather than what a widget wrote
    bool isRevert = false;
};

class PropertyRecorder
{
public:
    /// what a feature calls when its write path changes one entity's text
    static void RecordChange(Util::StringAtom extension, Game::Entity entity, const Util::String& path,
        const Util::String& oldText, const Util::String& newText);

    /// what a feature calls when its override mark is pressed. With no key it is the whole
    /// extension
    static void RequestRevert(Util::StringAtom extension, Game::Entity entity, const Util::String& path);

    /// what a feature asks when it draws a row, because the mark is the scene's and the feature
    /// cannot read it
    static bool IsAuthored(Util::StringAtom extension, Game::Entity entity, const Util::String& path);

    /// the editor is going away
    static void Discard();

private:
    /// the queued changes are run as commands, after the panels have drawn
    static void ApplyPending();

    static Util::Array<PendingPropertyChange> pending;
};

} // namespace Editor
//------------------------------------------------------------------------------
