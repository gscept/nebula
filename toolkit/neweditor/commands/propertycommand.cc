//------------------------------------------------------------------------------
//  @file propertycommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/propertycommand.h"
#include "game/levelextension.h"

namespace Editor
{

__ImplementClass(Editor::PropertyCommand, 'PRPC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
    What the undo stack says for the change. The cases are read from the path and from what the
    texts carry, because the command cannot read a text and only the extension knows which keys one
    holds.
*/
const char*
PropertyCommand::Name() const
{
    // An empty path is the whole entity, which is what the panel's Revert All runs through
    if (this->path.IsEmpty())
        return "Revert properties to prefab";

    const bool field = this->path.FindCharIndex('.') != InvalidIndex;

    if (this->isRevert)
        return field ? "Revert property field to prefab" : "Revert property to prefab";

    if (field)
        return "Set property field";

    // The path names the whole property, so the texts say whether the property was there before or
    // after. Only
    // the extension knows which keys a text carries, because the command cannot read one
    const Game::LevelExtension* extension = Game::LevelExtensionRegistry::GetExtension(this->extension);
    if (extension == nullptr || extension->keys == nullptr)
        return "Set property";

    const Util::String prefix = this->path + ".";

    Util::Array<Util::String> before;
    extension->keys(this->oldText.AsCharPtr(), before);

    bool had = false;
    for (IndexT i = 0; i < before.Size(); i++)
    {
        if (before[i].BeginsWithString(prefix))
        {
            had = true;
            break;
        }
    }
    if (!had)
        return "Add property";

    Util::Array<Util::String> after;
    extension->keys(this->newText.AsCharPtr(), after);

    bool has = false;
    for (IndexT i = 0; i < after.Size(); i++)
    {
        if (after[i].BeginsWithString(prefix))
        {
            has = true;
            break;
        }
    }
    if (!has)
        return "Remove property";

    return "Set property";
}

//------------------------------------------------------------------------------
/**
    The managed side worked out the text before and after but wrote nothing, so this is where the
    change happens. The text is applied through the extension that produced it, because the scene
    cannot read it, and the marker is written as well because the next save writes what the marker
    says rather than what the entity holds.
*/
bool
PropertyCommand::Execute()
{
    if (this->scene == nullptr)
        return false;

    const Game::LevelExtension* extension = Game::LevelExtensionRegistry::GetExtension(this->extension);
    if (extension == nullptr || extension->apply == nullptr)
        return false;

    if (!this->scene->ApplyExtension(this->entity, extension, this->newText))
        return false;

    this->scene->SetExtensionOverride(this->entity, this->extension, this->newFields);
    return true;
}

//------------------------------------------------------------------------------
/**
    The same, with the texts and the keys put back the other way. This is what makes a revert
    undoable: the marker has to go back too, otherwise the undo restores the value but leaves the
    field following the prefab.
*/
bool
PropertyCommand::Unexecute()
{
    if (this->scene == nullptr)
        return false;

    const Game::LevelExtension* extension = Game::LevelExtensionRegistry::GetExtension(this->extension);
    if (extension == nullptr || extension->apply == nullptr)
        return false;

    if (!this->scene->ApplyExtension(this->entity, extension, this->oldText))
        return false;

    this->scene->SetExtensionOverride(this->entity, this->extension, this->oldFields);
    return true;
}

} // namespace Editor
//------------------------------------------------------------------------------
