//------------------------------------------------------------------------------
//  @file scripteditor.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "scripteditor.h"

#if WITH_NEBULA_EDITOR
#include "game/entity.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/drawsurface.h"
#include "neweditor/extensions/propertyrecorder.h"
#include "neweditor/model/scene.h"
#include "nsharp/propertyextension.h"
#include "util/delegate.h"
#include "util/guid.h"
#include "util/hashtable.h"

namespace Scripting
{

//------------------------------------------------------------------------------
/**
    The heading and the rows, drawn through the surface so the section reads like the rest of the
    panel. Each property draws its own title row, with its remove button, so this is only the heading
    over them.

    The write path works out what the text would be after a field is written, and a text that names
    entities names them by the guids this scene's records have, so the map has to be the scene's own
    for the recorded text to read the same as the text the file would get.
*/
static void
DrawProperties(Game::Entity entity)
{
    Editor::Scene* scene = Editor::EditorFeatureRegistry::GetShell()->GetScene().get();

    Util::HashTable<Util::Guid, Game::Entity> map;
    scene->BuildSceneMap(map);
    scene->SetRefContext(map);

    Editor::DrawSection("Properties", nullptr, false);
    Scripting::PropertyLevelExtension::Draw(entity);
}

//------------------------------------------------------------------------------
/**
    The text a new property starts with. Only this side knows what a default instance of a managed type
    is.
*/
static void
DefaultText(const char* name, Util::String& out)
{
    Scripting::PropertyLevelExtension::DefaultPropertyJson(name, out);
}

//------------------------------------------------------------------------------
/**
    The panel has no way to read what a property is, so it only has the names this side mirrors. There
    is no namespace for a property, so all of them are one group.
*/
static void
CollectEntries(Game::Entity entity, Util::Array<Editor::InspectorEntry>& out)
{
    const Util::Array<Util::String>& names = Scripting::PropertyLevelExtension::PropertyNames();
    for (IndexT i = 0; i < names.Size(); i++)
    {
        Editor::InspectorEntry entry;
        entry.extension = Scripting::PropertyLevelExtension::ExtensionName();
        entry.name = names[i];
        entry.group = "Properties";
        entry.isComponent = false;
        entry.present = Scripting::PropertyLevelExtension::HasProperty(entity, names[i].AsCharPtr());
        entry.blocked = false;
        entry.defaultText =
            Util::Delegate<void(const char*, Util::String&)>::FromFunction<&DefaultText>();

        out.Append(entry);
    }
}

//------------------------------------------------------------------------------
/**
    The change the managed write path reported, handed to the recorder with the name it cannot work out
    on its own.
*/
static void
OnPropertyChange(uint64_t entityId, const char* path, const char* oldText, const char* newText)
{
    Editor::PropertyRecorder::RecordChange(Scripting::PropertyLevelExtension::ExtensionName(),
        Game::Entity(entityId), Util::String(path), Util::String(oldText), Util::String(newText));
}

//------------------------------------------------------------------------------
/**
*/
static void
OnRevertRequest(uint64_t entityId, const char* path)
{
    Editor::PropertyRecorder::RequestRevert(Scripting::PropertyLevelExtension::ExtensionName(),
        Game::Entity(entityId), Util::String(path));
}

//------------------------------------------------------------------------------
/**
*/
static bool
OnIsAuthored(uint64_t entityId, const char* path)
{
    return Editor::PropertyRecorder::IsAuthored(Scripting::PropertyLevelExtension::ExtensionName(),
        Game::Entity(entityId), Util::String(path));
}

//------------------------------------------------------------------------------
/**
    The feature registers what it has, and the editor reads the registry at open. A feature that
    registers nothing is never asked for, so this does not check whether an editor is running.
*/
void
RegisterEditorContributions()
{
    Editor::EditorFeatureRegistry::RegisterInspectorSection(
        Util::Delegate<void(Game::Entity)>::FromFunction<&DrawProperties>());
    Editor::EditorFeatureRegistry::RegisterInspectorAddEntries(
        Util::Delegate<void(Game::Entity, Util::Array<Editor::InspectorEntry>&)>::
            FromFunction<&CollectEntries>());

    // The recorder is generic and resolves the extension by name. What this side gives is the name,
    // and these three functions the managed write path reaches back through
    Scripting::PropertyLevelExtension::SetChangeCallback(&OnPropertyChange);
    Scripting::PropertyLevelExtension::SetRevertCallback(&OnRevertRequest);
    Scripting::PropertyLevelExtension::SetAuthoredCallback(&OnIsAuthored);
}

//------------------------------------------------------------------------------
/**
    The callbacks are taken back here, because the extension holds them as plain function pointers and
    has no way to know when this side goes away.
*/
void
ClearEditorContributions()
{
    Scripting::PropertyLevelExtension::SetChangeCallback(nullptr);
    Scripting::PropertyLevelExtension::SetRevertCallback(nullptr);
    Scripting::PropertyLevelExtension::SetAuthoredCallback(nullptr);
}

} // namespace Scripting
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
