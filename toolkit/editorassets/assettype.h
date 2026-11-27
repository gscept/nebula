#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::AssetTypeRegistry
    The asset types the editor knows. The asset side of the editor does not know what a model or a
    material is. A subsystem registers each type it owns, with what the editor needs to list it,
    make one and open one, and the browser, the asset editor and the import read the registry.
    A type is named by the type its URN carries, folder:file:type, so 'mdl' is the model. Registering
    a type also maps that name to the extension of its work file, which is what turns a URN into the
    file the asset is authored in. The export side of the mapping is the runtime's, because a game
    loads the exported files without the editor.
    Everything but the names is optional. A type without create cannot be made from nothing, and a
    type without open is listed but has no editor yet.
    The registry is static, like the editor's own, so a subsystem can register before the editor
    opens whatever order the features come in.
    @see
    Editor::EditorFeatureRegistry
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "io/path.h"
#include "util/array.h"
#include "util/delegate.h"
#include "util/string.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
    One asset type and what the editor can do with it.
*/
struct AssetType
{
    /// the type the URN names, like 'mdl'
    Util::String type;
    /// the extension of the work file, without the dot, like 'nasset'
    Util::String workExtension;
    /// the name lists and menus show, like 'Model'
    Util::String name;
    /// the icon glyph, by its name in the editor's icon table
    Util::String icon;
    /// the other URN types the asset is exported as, under the same folder and file. A model is
    /// also its meshes ('msh'), and when it has them its animation ('ani'), skeleton ('ske') and
    /// physics ('phys'), so a field that takes one of those takes the model
    Util::Array<Util::String> exports;

    /// the kinds a new asset of the type can start as, the material templates for a material. A
    /// type with one kind leaves this unset
    Util::Delegate<void(Util::Array<Util::String>& out)> variants;
    /// write the work file of a new asset to the given file. The variant is one of the kinds, empty
    /// for a type with one kind. Return false and say why when the file could not be written
    Util::Delegate<bool(const IO::Path& asset, const IO::URI& workFile, const Util::String& variant, Util::String& outError)> create;
    /// open the asset in its editor
    Util::Delegate<void(const IO::Path& asset)> open;
    /// import the asset again from its source files. Return false and say why when it has none
    Util::Delegate<bool(const IO::Path& asset, Util::String& outError)> reimport;
};

//------------------------------------------------------------------------------
/**
*/
class AssetTypeRegistry
{
public:
    /// Register a type, or replace one registered under the same name. The work mapping goes in
    /// with it.
    static void Register(const AssetType& type);
    /// Take a type back. A subsystem does this when it stops, because the delegates point into it.
    /// The work mapping stays, since another part may still resolve files with it.
    static void Unregister(const Util::String& type);
    /// Return the type the name refers to, or nullptr when nothing registered it.
    static const AssetType* Find(const Util::String& type);
    /// Return the type of an asset, by the type its URN carries, or nullptr for a folder or an
    /// unknown type.
    static const AssetType* Find(const IO::Path& asset);
    /// Return every registered type, in the order they were registered.
    static const Util::Array<AssetType>& GetTypes();
    /// True if an asset of the type can stand for one of the wanted type: it is of that type, or
    /// it is exported as it too. An empty wanted type takes any.
    static bool Provides(const Util::String& type, const Util::String& wanted);
    /// The asset as the wanted type, the same folder and file under the wanted type when the asset
    /// is exported as it. Returns false when the asset is not of the type, or its type could export
    /// it but this asset has no such export, a model without a skeleton for 'ske'. An empty wanted
    /// type takes any asset as it is.
    static bool As(const IO::Path& asset, const Util::String& wanted, IO::Path& outAsset);
    /// The registered type an asset is exported from, the model of a 'ske'. Returns the type of the
    /// asset itself if it is registered, nullptr if no type has it.
    static const AssetType* FindSource(const IO::Path& asset, IO::Path& outSource);

private:
    static Util::Array<AssetType> types;
};

/// The URN (folder:file:type) of an exported asset from the name it's loaded by, an export path like
/// tex:folder/file.dds, the local path it resolves to, or a URN. Returns false for assets that aren't
/// exported from the work folder
bool AssetURN(const Util::String& resourceName, const char* type, Util::String& outURN);

} // namespace Editor
//------------------------------------------------------------------------------
