#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AssetProcessorBase
    
    Implements a base class for an asset processor
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/uri.h"
#include "toolkit-common/platform.h"
#include "toolkit-common/logger.h"
#include "net/socket/socket.h"
#include "io/console.h"
#include "db/database.h"
#include "db/dbfactory.h"

typedef void (*AssetProcessorProgressCallback) (float progress, const Util::String& status);
typedef void (*AssetProcessorMinMaxCallback) (int min, int max);
//------------------------------------------------------------------------------
namespace Base
{
class AssetProcessorBase : public Core::RefCounted
{
    __DeclareClass(AssetProcessorBase);

public:
    enum ExportFlag
    {
        All,
        Dir,
        File
    };

    /// constructor
    AssetProcessorBase();
    /// destructor
    virtual ~AssetProcessorBase();

    /// opens the exporter
    virtual void Open();
    /// closes the exporter
    virtual void Close();
    /// returns true if exporter is open
    bool IsOpen() const;

    /// call this after exporting a file. This will generate an intermediate file that is used to keep track of source <-> export linkage
    void WriteIntermediateFile(const IO::URI& sourceFile, Util::Array<IO::URI> const& output);

    /// exports a single file
    virtual void ProcessFile(const IO::URI& file);
    /// exports a single directory
    virtual void ProcessDir(const Util::String& category);
    /// exports all files
    virtual void ProcessAll();

    /// sets error flag
    void SetHasErrors(bool flag);
    /// returns error flag
    const bool HasErrors() const;

    /// sets the category
    void SetFolder(const Util::String& folder);
    /// sets the file name
    void SetFile(const Util::String& file);
    /// sets the platform
    void SetPlatform(ToolkitUtil::Platform::Code platform);
    /// sets the parse flag for the parser
    void SetExportFlag(ExportFlag parseFlag);
    /// sets if the parser should force export
    void SetForce(bool force);
    /// sets if the parser should be used as a remote tool
    void SetRemote(bool remote);
    /// sets the exporter callback (only used if remote is false)
    void SetProgressCallback(AssetProcessorProgressCallback callback);
    /// sets the min-max callback (only used if remote is false)
    void SetMinMaxCallback(AssetProcessorMinMaxCallback callback);
    /// Set the logger
    void SetLogger(ToolkitUtil::Logger* logger);

    /// helper function for reporting progress
    void Progress(float progress, const Util::String& status);
    /// helper function for sending the progress min and max values
    void SetProgressMinMax(int min, int max);

    /// counts number of files to export
    int CountExports(const Util::String& dir, const Util::String& ext);

    /// sets progress precision
    void SetProgressPrecision(int precision);

    /// Update resource mappings
    void UpdateResourceMapping(Util::String urn, Util::String work, Util::String exp);

protected:

    /// reports an error, depending on what read state we are in, the error will be fatal or a warning
    void ReportError(const char* error, ...);
    /// checks whether or not a file needs to be updated 
    bool NeedsConversion(const Util::String& src, const Util::String& dst);
    /// validate the intermediate file and fix any problems
    void ValidateIntermediateFile(Util::String const& filePath);
    /// Recursively validate all intermediate files in directory and subdirs
    void RecurseValidateIntermediates(Util::String const& dir);

    Util::String folder;
    Util::String file;

    int precision;
    Ptr<Net::Socket> socket;
    ToolkitUtil::Platform::Code platform;
    ExportFlag exportFlag;
    AssetProcessorProgressCallback progressCallback;
    AssetProcessorMinMaxCallback  minMaxCallback;
    ToolkitUtil::Logger* logger;

    Ptr<Db::Database> database;
    Ptr<Db::DbFactory> dbFactory;

    bool hasErrors;

    bool force;
    bool isOpen;
    bool remote;
}; 

//------------------------------------------------------------------------------
/**
*/
inline bool 
AssetProcessorBase::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AssetProcessorBase::SetHasErrors( bool flag )
{
    this->hasErrors = flag;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool 
AssetProcessorBase::HasErrors() const
{
    return this->hasErrors;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AssetProcessorBase::SetProgressPrecision( int precision )
{
    this->precision = precision;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AssetProcessorBase::SetFolder( const Util::String& folder )
{
    this->folder = folder;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AssetProcessorBase::SetFile( const Util::String& file )
{
    this->file = file;
    this->file.StripFileExtension();
}


//------------------------------------------------------------------------------
/**
*/
inline void 
AssetProcessorBase::SetPlatform( ToolkitUtil::Platform::Code platform )
{
    this->platform = platform;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AssetProcessorBase::SetExportFlag( ExportFlag exportFlag )
{
    this->exportFlag = exportFlag;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AssetProcessorBase::SetForce( bool force )
{
    this->force = force;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AssetProcessorBase::SetRemote( bool remote )
{
    this->remote = remote;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
AssetProcessorBase::SetProgressCallback( AssetProcessorProgressCallback callback )
{
    this->progressCallback = callback;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
AssetProcessorBase::SetMinMaxCallback( AssetProcessorMinMaxCallback callback )
{
    this->minMaxCallback = callback;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AssetProcessorBase::SetLogger(ToolkitUtil::Logger* logger)
{
    this->logger = logger;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AssetProcessorBase::ReportError( const char* error, ... )
{
    va_list argList;
    va_start(argList, error);
    if (this->exportFlag == AssetProcessorBase::File)
    {
        IO::Console::Instance()->Error(error, argList);
    }
    else
    {
        IO::Console::Instance()->Warning(error, argList);
    }
    va_end(argList);
}


} // namespace ToolkitUtil
//------------------------------------------------------------------------------