#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AssetFile
    
    Represents a single file in an AssetRegistry.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "io/filetime.h"
#include "util/stringatom.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class AssetFile
{
public:
    /// asset file states
    enum State
    {
        Unknown,            // state hasn't been determined yet
        New,                // asset file is new
        Deleted,            // asset file no longer exists
        Changed,            // asset file has changed since the last scan
        Unchanged,          // asset file was unchaged since the last scan
        Touched,            // asset file needs a time stamp update
        Error,              // an error occured during scanning
    };

    /// default constructor
    AssetFile();
    
    /// set asset file path
    void SetPath(const Util::StringAtom& path);
    /// get asset file path
    const Util::StringAtom& GetPath() const;
    /// set modification time (when the asset has been exported)
    void SetTimeStamp(IO::FileTime fileTime);
    /// get modification time
    IO::FileTime GetTimeStamp() const;
    /// set checksum
    void SetChecksum(uint crc);
    /// get checksum
    uint GetChecksum() const;
    /// set current state of the asset file
    void SetState(State s);
    /// get current state
    State GetState() const;
    
private:
    Util::StringAtom path;
    IO::FileTime timeStamp;
    uint checksum;    
    State state;
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
