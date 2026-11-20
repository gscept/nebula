#pragma once
//------------------------------------------------------------------------------
/**
    @class Scripting::PropertyLevelExtension

    The level formats' contribution for managed properties. This is the side that knows what the json
    means, and the formats only know the extension by name.

    The managed entry points are resolved once at registration and cached here, which is what keeps the
    level format from resolving them through hostfxr for every entity it writes.

    The text is the entity as one node per property. It is the only shape the formats can use, because
    they cannot read the text and cannot group it, and it is the shape an editor command holds as well:
    a command applies the entity's whole text, which is what makes a change that took a property off read
    as a change rather than as an addition.

    What an editor knows that this side cannot read is handed over as callbacks: the change the managed
    write path reported, the key a marker was pressed on, and whether a row is the instance's own. These
    are the editor's half of the contract, and they are null in a build that has no editor.

    @see Game::LevelExtension
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/levelextension.h"
#include "game/entity.h"
#include "util/array.h"
#include "util/string.h"
#include "util/stringatom.h"
#include "assemblyid.h"

//------------------------------------------------------------------------------
namespace Scripting
{

class NSharpServer;

//------------------------------------------------------------------------------
/**
*/
class PropertyLevelExtension
{
public:
    /// what an editor hands the extension when it attaches. The managed write path calls the exported
    /// RecordPropertyChange, which comes here. Null when nothing is attached, which is what makes the
    /// managed call a no-op in a build that has no editor
    using ChangeFunc = void(*)(uint64_t entityId, const char* path, const char* oldText, const char* newText);
    static void SetChangeCallback(ChangeFunc func);

    /// what an editor hands the extension for a revert request. The managed side can only say which key was
    /// pressed on; what a press means is the editor's decision, and this is the function it answers with
    using RevertFunc = void(*)(uint64_t entityId, const char* path);
    static void SetRevertCallback(RevertFunc func);

    /// what an editor hands the extension for the override marker. The marker is the editor's own and this
    /// side cannot read it, so a row asks rather than being told. Null when nothing is attached, which is
    /// what makes the managed call a no-op in a build that has no editor
    using AuthoredFunc = bool(*)(uint64_t entityId, const char* path);
    static void SetAuthoredCallback(AuthoredFunc func);

    /// resolve the managed entry points and register the extension with the level formats
    static void Register(Scripting::NSharpServer* server, Scripting::AssemblyId assemblyId);

    /// the json for one entity, as the managed runtime produces it
    static void Capture(Game::Entity entity, Util::String& out);

    /// one entity, put back from the json the file carried for it
    static void Apply(Game::Entity entity, const char* json);

    // -- The merge contract. The level formats cannot read what this extension produces, so these are the
    // only things they can say about it --

    /// the json for one entity with only the named keys, which is what a linked entity writes
    static void CaptureKeys(Game::Entity entity, const Util::Array<Util::String>& keys, Util::String& out);

    /// the keys a text carries, as the paths this extension names them by
    static void TextKeys(const char* text, Util::Array<Util::String>& out);

    /// what the base text gives when the keys the other text carries are put over it
    static void Merge(const char* base, const char* over, Util::String& out);

    /// the text for the named keys, each value read from the base text, or the field's default when the base
    /// has nothing. This is what a revert to the prefab writes, because a key the chain does not carry has no
    /// value to put back
    static void Resolve(const char* base, const Util::Array<Util::String>& keys, Util::String& out);

    /// the file's guid map, as "guid:entityId" pairs. Only valid for the duration of the call that sets it,
    /// which is what makes a merged text resolve against the files it names guids from
    static void SetRefContext(const char* pairs);

    // -- The editor contract --

    /// the property section, drawn by the managed side through the editor's ImGui surface. Only called from
    /// inside the editor's frame, which is the only place the surface is reachable from
    static void Draw(Game::Entity entity);

    /// the name the extension registered under, which is what the editor's override marker is keyed by
    static Util::StringAtom ExtensionName();

    /// the change the managed write path reported. Runs the callback, which is what builds the command. The
    /// path is the key inside the text the change is about, which is the only place the editor ever learns
    /// what a key means, and a path with no field is the whole property
    static void RecordChange(Game::Entity entity, const char* path, const char* oldText, const char* newText);

    /// the key the managed side pressed the override marker on. Runs the callback, which is what builds the
    /// revert. A path with no field is the whole property
    static void RequestRevert(Game::Entity entity, const char* path);

    /// whether the editor's marker says this entity authored the path. A path with no field asks whether any
    /// field of the property is authored, which is what a section row answers with
    static bool IsAuthored(Game::Entity entity, const char* path);

    // -- The menu contract --

    // The names the editor's dropdown lists. The panel does not know what a property is, so the managed side
    // mirrors its registry into here. The mirror is rebuilt from the managed side on every change, because an
    // assembly can be registered more than once
    static void AddPropertyName(const char* propertyName);
    static void ClearPropertyNames();
    static const Util::Array<Util::String>& PropertyNames();

    /// true when the entity has the property, which is what makes a dropdown row read as already added
    static bool HasProperty(Game::Entity entity, const char* propertyName);

    /// the text a default instance of the property produces, which is what an add writes
    static void DefaultPropertyJson(const char* propertyName, Util::String& out);

private:
    static void* serializeProperties;
    static void* applyProperties;
    static void* drawProperties;
    static void* hasProperty;
    static void* defaultPropertyJson;
    static void* serializeKeys;
    static void* jsonKeys;
    static void* mergeJson;
    static void* resolveJson;
    static void* setRefContext;

    static ChangeFunc changeCallback;
    static RevertFunc revertCallback;
    static AuthoredFunc authoredCallback;

    // A feature unit can be attached more than once, and the registry refuses two extensions with the
    // same name, so the extension is only registered the first time
    static bool registered;

    // The name the extension registered under, which is what the editor's override marker is keyed by
    static Util::StringAtom extensionName;

    // The buffer the managed side writes into. Owned here, because the delegate writes into it directly
    static Util::Array<char> buffer;

    // The names the managed side mirrored. Owned here, because the panel reads them while it draws
    static Util::Array<Util::String> names;
};

} // namespace Scripting
//------------------------------------------------------------------------------
