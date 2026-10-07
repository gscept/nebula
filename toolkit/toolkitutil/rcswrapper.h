#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::RCSWrapper
    
    A transparent wrapper around CVS or SVN, only supports updating.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "logger.h"
#include "io/stream.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class RCSWrapper
{
public:
    /// constructor
    RCSWrapper();
    /// destructor
    ~RCSWrapper();

    /// set location of SVN tool
    void SetSVNToolLocation(const Util::String& path);
    /// get location of SVN tool
    const Util::String& GetSVNToolLocation() const;
    /// set directories for "commit timestamp" mode
    void SetCommitTimeStampDirectories(const Util::Array<Util::String>& paths);
    /// get "commit timestamp" directories
    const Util::Array<Util::String>& GetCommitTimeStampDirectories() const;
    /// set directories for "update timestamp" update mode
    void SetUpdateTimeStampDirectories(const Util::Array<Util::String>& paths);
    /// get "update timestamp" directories
    const Util::Array<Util::String>& GetUpdateTimeStampDirectories() const;
    /// perform update operation
    bool Update(Logger& logger);
    /// get modified files in local working copy
    bool GetModifiedFiles(Logger& logger, const Util::String& directory, Util::Array<Util::String>& outFiles) const;

private:
    /// write a temporary config file for SVN
    void WriteTempSVNConfigFiles();
    /// helper method to copy content of a directory
    void CopyDirectory(const Util::String& fromDir, const Util::String& toDir);
    /// update one directory
    bool UpdateDirectory(Logger& logger, const Util::String& directory, bool commitTimeStampMode);
    /// returns true, if the given directory is under SVN control
    bool IsSVNDirectory(Logger& logger, const Util::String& directory) const;
    /// returns true, if the SVN command line tool has been set and exits
    bool IsSVNCommandLineConfigured(Logger& logger) const;
    /// parse SVN status output for modified files
    bool ParseSVNStatusOutput(const Ptr<IO::Stream>& stream, const Util::String& directory, Util::Array<Util::String>& outFiles) const;

    Util::String svnToolLocation;
    Util::Array<Util::String> updateTimeStampDirs;
    Util::Array<Util::String> commitTimeStampDirs;
};

//------------------------------------------------------------------------------
/**
*/
inline void
RCSWrapper::SetSVNToolLocation(const Util::String& path)
{
    this->svnToolLocation = path;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
RCSWrapper::GetSVNToolLocation() const
{
    return this->svnToolLocation;
}

//------------------------------------------------------------------------------
/**
*/
inline void
RCSWrapper::SetCommitTimeStampDirectories(const Util::Array<Util::String>& dirs)
{
    this->commitTimeStampDirs = dirs;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Util::String>&
RCSWrapper::GetCommitTimeStampDirectories() const
{
    return this->commitTimeStampDirs;
}

//------------------------------------------------------------------------------
/**
*/
inline void
RCSWrapper::SetUpdateTimeStampDirectories(const Util::Array<Util::String>& dirs)
{
    this->updateTimeStampDirs = dirs;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Util::String>&
RCSWrapper::GetUpdateTimeStampDirectories() const
{
    return this->updateTimeStampDirs;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    