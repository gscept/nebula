//------------------------------------------------------------------------------
//  @file assettype.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorassets/assettype.h"
#include "io/ioserver.h"

namespace Editor
{

Util::Array<AssetType> AssetTypeRegistry::types;

//------------------------------------------------------------------------------
/**
    A type registered again replaces the old entry in place, so the order the browser lists the
    types in does not change when a subsystem restarts.
*/
void
AssetTypeRegistry::Register(const AssetType& type)
{
    n_assert(type.type.IsValid());
    n_assert(type.workExtension.IsValid());

    IO::Path::AddWorkMapping(type.type, type.workExtension);

    for (IndexT i = 0; i < types.Size(); i++)
    {
        if (types[i].type == type.type)
        {
            types[i] = type;
            return;
        }
    }
    types.Append(type);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetTypeRegistry::Unregister(const Util::String& type)
{
    for (IndexT i = 0; i < types.Size(); i++)
    {
        if (types[i].type == type)
        {
            types.EraseIndex(i);
            return;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
const AssetType*
AssetTypeRegistry::Find(const Util::String& type)
{
    for (IndexT i = 0; i < types.Size(); i++)
    {
        if (types[i].type == type)
            return &types[i];
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
const AssetType*
AssetTypeRegistry::Find(const IO::Path& asset)
{
    if (!asset.IsFile())
        return nullptr;
    return Find(asset.GetType());
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<AssetType>&
AssetTypeRegistry::GetTypes()
{
    return types;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetTypeRegistry::Provides(const Util::String& type, const Util::String& wanted)
{
    if (wanted.IsEmpty() || type == wanted)
        return true;
    const AssetType* assetType = Find(type);
    return assetType != nullptr && assetType->exports.FindIndex(wanted) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    An asset of the wanted type is taken as it is, it is in the index because it exists. An export
    of another type is only there when the asset has what it is made from, so it is looked for.
*/
bool
AssetTypeRegistry::As(const IO::Path& asset, const Util::String& wanted, IO::Path& outAsset)
{
    if (!asset.IsFile() || asset.GetType().IsEmpty())
        return false;
    if (wanted.IsEmpty() || asset.GetType() == wanted)
    {
        outAsset = asset;
        return true;
    }
    if (!Provides(asset.GetType(), wanted))
        return false;

    const IO::Path exported = IO::Path::File(asset.GetFolder(), asset.GetFile(), wanted);
    if (!IO::Path::ExportExtensions.Contains(Util::StringAtom(wanted)) || !IO::FileExists(exported.ExportURI()))
        return false;
    outAsset = exported;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
const AssetType*
AssetTypeRegistry::FindSource(const IO::Path& asset, IO::Path& outSource)
{
    if (!asset.IsFile())
        return nullptr;
    if (const AssetType* own = Find(asset.GetType()))
    {
        outSource = asset;
        return own;
    }
    for (IndexT i = 0; i < types.Size(); i++)
    {
        if (types[i].exports.FindIndex(asset.GetType()) != InvalidIndex)
        {
            outSource = IO::Path::File(asset.GetFolder(), asset.GetFile(), types[i].type);
            return &types[i];
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetURN(const Util::String& resourceName, const char* type, Util::String& outURN)
{
    const IO::Path path = IO::Path::Parse(resourceName);
    if (path.IsFile() && path.GetType() == type)
    {
        outURN = resourceName;
        return true;
    }

    // an export lives below the folder its type's assign names, the rest of the path is the folder and file
    Util::String root = IO::URI(Util::Format("%s:", type)).LocalPath();
    root.TrimRight("/\\");
    const Util::String file = IO::URI(resourceName).LocalPath();
    if (file.Length() <= root.Length() + 1 || !file.BeginsWithString(root) || (file[root.Length()] != '/' && file[root.Length()] != '\\'))
        return false;
    outURN = IO::Path::FolderAndFile(type, file.ExtractToEnd(root.Length() + 1)).AsString();
    return true;
}

} // namespace Editor
