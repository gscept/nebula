#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::LevelExtension

    A feature's contribution to the level formats. The formats know an extension by name and treat
    what it produces as text they do not interpret, which is what lets a feature serialize something
    the engine has no type knowledge of.

    The formats do not need the extension to write or read: they write the text capture produced as a
    node under the entity, and they hand the text they read back to apply. That is also what makes an
    extension the file carries but this build does not have survive a save, because the text is kept
    whether or not an extension can be found for it.

    The hooks the merge needs are the ones that let the formats say something about text they
    cannot read: which keys the text carries, what the text looks like with only those keys, what
    one text gives when another text is put over it, and what one text gives for a named key when it
    does not carry it. They are used the way component reflection is used for a component's fields,
    and an extension without them is written whole, which is what a feature that has no authored
    fields gets.

    Every hook that writes a text writes it in the same order: the nodes by name and the keys inside a
    node by name. The formats compare texts as bytes, because they cannot read them, and an order that
    differed between the hooks would read as a change that no value changed.

    One text, one shape: the entity as one node per feature thing. The formats cannot read the text,
    so they cannot group it, and that is the only shape a file can carry. Everything that composes a
    text works in this shape, and an editor command holds this shape as well.

    The formats also hand the extension the file's guid map, because the extension cannot resolve what
    its own text names. A text that names entities is written as the guids the file names them by, and
    the map is only valid for the duration of the one call that needs it.

    A text that names entities is the one thing the formats cannot do on their own, because they cannot
    read the text. They know the guids a text names only when the extension says them, and they can put
    other guids into the text only when the extension writes them. That is what the two reference hooks
    are, and they are what makes an extension's references go through the same document as a
    component's: the document records them, and the scene patches and remaps them the way it does for a
    component field.

    @see Game::LevelExtensionRegistry
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/entity.h"
#include "util/array.h"
#include "util/dictionary.h"
#include "util/guid.h"
#include "util/string.h"
#include "util/stringatom.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
    One entity reference a text names: the path the extension names its field by, and the guid the text
    carries for the target. The formats cannot see inside a text, so this is the shape they can hold a
    reference in.
*/
struct TextRef
{
    Util::String path;
    Util::Guid guid;
};

//------------------------------------------------------------------------------
/**
*/
class LevelExtension
{
public:
    using CaptureFunc = void(*)(Game::Entity, Util::String&);
    /// the entity, put back from the text. Exact: what the text names is what the entity has, and what it
    /// does not name is taken off. A command holds the entity's whole text, so this is what makes a change
    /// that took one thing off read as a change rather than as an addition
    using ApplyFunc   = void(*)(Game::Entity, const char*);

    /// the text with only the named keys, which is what a linked entity writes and what a merge composes
    /// from. Null when the extension has no keys to answer with
    using CaptureKeysFunc = void(*)(Game::Entity, const Util::Array<Util::String>& keys, Util::String& out);
    /// the keys the given text carries. The formats cannot see inside the text, so this is how they know
    /// which part of it an entity authored
    using KeysFunc = void(*)(const char* text, Util::Array<Util::String>& out);
    /// what the base text gives when the keys the other text carries are put over it
    using MergeFunc = void(*)(const char* base, const char* over, Util::String& out);
    /// the text for the named keys, each value read from the given base text, or the field's default when the
    /// base has nothing. A revert to the prefab needs this, because a key the base does not carry has no value
    /// to put back, and only the extension knows what its field's default is
    using ResolveFunc = void(*)(const char* base, const Util::Array<Util::String>& keys, Util::String& out);
    /// the file's guid map as "guid:entityId" pairs, for the duration of one capture or apply
    using RefContextFunc = void(*)(const char* pairs);
    /// the entity references the text names, as the paths the extension names its fields by. The
    /// formats cannot read a text, so this is how they learn which parts of it name entities, and it is
    /// what lets a document record an extension reference the way it records a component one
    using RefsFunc = void(*)(const char* text, Util::Array<Game::TextRef>& out);
    /// the text with the named paths' references put as the given guids, as "path\tguid" pairs. A text
    /// that names entities can only be written into a file that has a record for them, and this is what
    /// lets the formats move a text from one file's guids to another's. A path the text does not name is
    /// left alone, and a path given with no guid is written as nothing
    using RemapFunc = void(*)(const char* text, const char* pairs, Util::String& out);

    /// the node name the extension writes under an entity
    Util::StringAtom name;
    /// the text for one entity, produced from what the feature holds
    CaptureFunc capture = nullptr;
    /// one entity, put back from the text the file carried for it
    ApplyFunc apply = nullptr;
    CaptureKeysFunc captureKeys = nullptr;
    KeysFunc keys = nullptr;
    MergeFunc merge = nullptr;
    ResolveFunc resolve = nullptr;
    RefContextFunc refContext = nullptr;
    RefsFunc refs = nullptr;
    RemapFunc remap = nullptr;
};

//------------------------------------------------------------------------------
/**
    The extensions the level formats write and read. Registered by feature units, which is what keeps
    the formats from knowing anything about what a feature serializes.
*/
class LevelExtensionRegistry
{
public:
    /// register an extension. The name is what the formats use to find it again
    static void Register(const LevelExtension& extension);
    /// the extension registered under a name, or null when there is none
    static const LevelExtension* GetExtension(Util::StringAtom name);
    /// all extensions, in the order they were registered, which is the order the formats write them in
    static const Util::Array<LevelExtension>& GetAllExtensions();

private:
    static LevelExtensionRegistry* Instance();
    static void Destroy();

    LevelExtensionRegistry();
    ~LevelExtensionRegistry();

    static LevelExtensionRegistry* Singleton;

    Util::Array<LevelExtension> extensions;
    Util::Dictionary<Util::StringAtom, IndexT> byName;
};

} // namespace Game
//------------------------------------------------------------------------------
