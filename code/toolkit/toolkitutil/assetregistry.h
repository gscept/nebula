#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AssetRegistry
    
    An asset registry is a persistent database for additional asset file
    attributes (currently: last modification time and a crc checksum). It's
    used by the sync tool to determine whether an asset is new, obsolete
    or has been modified since the last sync operation.    
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "toolkitutil/assetfile.h"
#include "toolkitutil/logger.h"
#include "util/dictionary.h"
#include "timing/timer.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class AssetRegistry
{
public:
    /// constructor
    AssetRegistry();
    
    /// set the root location (i.e. "proj:")
    void SetRootPath(const Util::String& path);
    /// get the root location
    const Util::String& GetRootPath() const;
    /// set location of the asset registry file
    void SetRegistryFile(const Util::String& path);
    /// get location of the asset registry file
    const Util::String& GetRegistryFile() const;
    /// set asset directories which should be parsed recursively
    void SetAssetDirectories(const Util::Array<Util::String>& dirs);
    /// get asset directories
    const Util::Array<Util::String>& GetAssetDirectories() const;
    
    /// update the registry in client mode (load reg file, scans files, saves reg file)
    bool UpdateLocal(Logger& logger);
    /// update as remote registry (only load the reg file)
    bool UpdateRemote(Logger& logger);
    /// save registry back into registry file
    bool SaveRegistry(Logger& logger);
    /// set this registry to the difference between a remote and a local registry
    bool BuildDifference(Logger& logger, AssetRegistry& localRegistry, const AssetRegistry& remoteRegistry);

    /// get number of entries in the registry
    SizeT GetNumAssets() const;
    /// get an asset at specified index
    const AssetFile& GetAssetByIndex(IndexT i) const;

private:
    /// load registry file
    bool LoadRegistryFile(Logger& logger, const IO::URI& uri);
    /// save registry file
    bool SaveRegistryFile(Logger& logger, const IO::URI& uri);
    /// scan asset directories and update registry
    bool UpdateRegistry(Logger& logger);
    /// scan asset files and recurse into subdirectories
    bool RecurseUpdateRegistry(Logger& logger, const Util::String& dir);
    /// add an asset file which doesn't exist yet in the registry
    void AddAssetFile(Logger& logger, const Util::StringAtom& filePath);
    /// update an asset file which already exists in the registry
    void UpdateAssetFile(Logger& logger, const Util::StringAtom& filePath);
    /// remove files from registry which no longer exist in file system
    void RemoveObsoleteFilesFromRegistry(Logger& logger);

    Util::String rootPath;
    Util::String registryPath;
    Util::Array<Util::String> directories;
    Util::Dictionary<Util::StringAtom, AssetFile> registry;
    SizeT numScannedFiles;
    SizeT numAddedFiles;
    SizeT numChangedFiles;
    SizeT numIdenticalFiles;
    SizeT numDeletedFiles;
    SizeT numTouchedFiles;
    SizeT numErrorFiles;
    Timing::Timer updateLocalTimer;
};

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AssetRegistry::GetNumAssets() const
{
    return this->registry.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline const AssetFile&
AssetRegistry::GetAssetByIndex(IndexT i) const
{
    return this->registry.ValueAtIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
inline void
AssetRegistry::SetRootPath(const Util::String& path)
{
    this->rootPath = path;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AssetRegistry::GetRootPath() const
{
    return this->rootPath;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AssetRegistry::SetRegistryFile(const Util::String& path)
{
    this->registryPath = path;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AssetRegistry::GetRegistryFile() const
{
    return this->registryPath;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AssetRegistry::SetAssetDirectories(const Util::Array<Util::String>& dirs)
{
    this->directories = dirs;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Util::String>&
AssetRegistry::GetAssetDirectories() const
{
    return this->directories;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    