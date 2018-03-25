#pragma once
//------------------------------------------------------------------------------
/**
	@class DistributedTools::DistributedAppJob

    A distributed application job contains information about an application,
    the commandline arguments that should be used, when the application is
    launched and other settings that are required to run the job.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "distributedtools/distributedjobs/distributedjob.h"

//------------------------------------------------------------------------------
namespace DistributedTools
{
    class DistributedAppJob : public DistributedTools::DistributedJob
{
__DeclareClass(DistributedTools::DistributedAppJob);
public:
    
    /// Constructor
    DistributedAppJob();
    /// Distructor
    virtual ~DistributedAppJob();
    

    void UpdateProjectDirectory();
    /// checks job setup and returns true if job is able to run on current environment
    bool IsAbleToRun();
    /// runs the application of the job
    void RunJob();
    /// get the output stream of a job
    const Ptr<IO::Stream> & GetOutputStream();
    /// set file list
    void SetFileList(const Util::Array<Util::String> & args);
    /// get file list
    const Util::Array<Util::String> & GetFileList();
    /// set file list
    void SetDirList(const Util::Array<Util::String> & args);
    /// get file list
    const Util::Array<Util::String> & GetDirList();
    /// set application path
    void SetApplicationPath(const Util::String & path);
    /// get application path
    const Util::String & GetApplicationPath();
    /// set arguments
    void SetArguments(const Util::String & args);
    /// get arguments
    const Util::String & GetArguments();
    /// clones of this job (with a different guid)
    virtual Ptr<DistributedJob> Clone();
    /// copy the attributes of another job to itself, except the guid
    virtual void CopyFrom(const Ptr<DistributedAppJob> & job);

private:
    /// called by GenerateJobCommand
    void WriteXmlContent(const Ptr<IO::XmlWriter> & writer);
    /// called by SetupFromXml
    void ReadXmlContent(const Ptr<IO::XmlReader> & reader);
    /// called by Update() when running job application
    void UpdateJob();

    IO::URI localListPath;
    Util::Array<Util::String> fileList;
    Util::Array<Util::String> dirList;

    Util::String applicationPath;
    bool appReportedError;
    ToolkitUtil::AppLauncher jobApplication;

    Util::String arguments;
    Ptr<IO::MemoryStream> appOutStream;
};

//------------------------------------------------------------------------------
/**
    Set file list for this job
*/
inline
void
DistributedAppJob::SetFileList(const Util::Array<Util::String> & list)
{
    n_assert(Ready == this->jobState);
    this->fileList.Clear();
    this->fileList.AppendArray(list);
}

//------------------------------------------------------------------------------
/**
    Get file list for this job
*/
inline
const Util::Array<Util::String> &
DistributedAppJob::GetFileList()
{
    return this->fileList;
}

//------------------------------------------------------------------------------
/**
    Set file list for this job
*/
inline
void
DistributedAppJob::SetDirList(const Util::Array<Util::String> & list)
{
    n_assert(Ready == this->jobState);
    this->dirList.Clear();
    this->dirList.AppendArray(list);
}

//------------------------------------------------------------------------------
/**
    Get file list for this job
*/
inline
const Util::Array<Util::String> &
DistributedAppJob::GetDirList()
{
    return this->dirList;
}

//------------------------------------------------------------------------------
/**
    Set application path for this job
*/
inline
void
DistributedAppJob::SetApplicationPath(const Util::String & path)
{
    n_assert(Ready == this->jobState);
    this->applicationPath = path;
}

//------------------------------------------------------------------------------
/**
    Get application path for this job
*/
inline
const Util::String &
DistributedAppJob::GetApplicationPath()
{
    return this->applicationPath;
}

//------------------------------------------------------------------------------
/**
    Set command line arguments for this job
*/
inline
void
DistributedAppJob::SetArguments(const Util::String & args)
{
    n_assert(Ready == this->jobState);
    this->arguments = args;
}

//------------------------------------------------------------------------------
/**
    Get command line arguments for this job
*/
inline
const Util::String &
DistributedAppJob::GetArguments()
{
    return this->arguments;
}

} // namespace DistributedTools