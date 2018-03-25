#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AssetUpdater
    
    This class takes a difference AssetRegistry set and realizes the
    necessary actions to synchronize the remote and local asset repository
    (copying new and changed files from the remote build server, deleting
    files which no longer exist in the build).
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "toolkitutil/assetregistry.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class AssetUpdater
{
public:
    /// constructor
    AssetUpdater();

    /// set the local root path (i.e. "proj:")
    void SetLocalRootPath(const Util::String& path);
    /// get the local root path
    const Util::String& GetLocalRootPath() const;
    /// set the remote root path (i.e. "remote:")
    void SetRemoteRootPath(const Util::String& path);
    /// get the remote root path
    const Util::String& GetRemoteRootPath() const;

    /// update the local asset repository
    bool Update(Logger& logger, const AssetRegistry& diffSet);

private:
    Util::String localRootPath;
    Util::String remoteRootPath;
};

//------------------------------------------------------------------------------
/**
*/
inline void
AssetUpdater::SetLocalRootPath(const Util::String& path)
{
    this->localRootPath = path;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AssetUpdater::GetLocalRootPath() const
{
    return this->localRootPath;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AssetUpdater::SetRemoteRootPath(const Util::String& path)
{
    this->remoteRootPath = path;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AssetUpdater::GetRemoteRootPath() const
{
    return this->remoteRootPath;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------

