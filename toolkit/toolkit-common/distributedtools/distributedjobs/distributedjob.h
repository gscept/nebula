#pragma once
//------------------------------------------------------------------------------
/**
    @class DistributedTools::DistributedJob

    Basic implementation of a distributed job.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "io/uri.h"
#include "io/memorystream.h"
#include "io/xmlwriter.h"
#include "io/xmlreader.h"
#include "toolkit-common/applauncher.h"
#include "util/guid.h"

//------------------------------------------------------------------------------
namespace DistributedTools
{
class DistributedJob : public Core::RefCounted
{
__DeclareClass(DistributedTools::DistributedJob);
public:
    
    enum DistributedJobState
    {
        Ready,      // job is ready to start
        ValidProjectPath, // the local project path has been validated
        Updating,   // job is updating to matching revision
        UpToDate,   // revision is up-to-date
        Working,    // job is running application
        Finished,   // job has finished and can be destroyed
        JobError    // an error has occoured, job should not continue
    };

    /// Constructor
    DistributedJob();
    /// Distructor
    virtual ~DistributedJob();
    
    /// set project directory
    void SetProjectDirectory(const Util::String & dir);
    /// get project directory
    const Util::String & GetProjectDirectory();
    /// set svn path
    void SetSVNPath(const Util::String & path);
    /// get svn path
    const Util::String & GetSVNPath();
    /// set required revision
    void SetRequiredRevision(const Util::String & revision);
    /// get required revision
    const Util::String & GetRequiredRevision();
    /// get state of the job
    DistributedJobState GetCurrentState();
    /// set simple identifier of job (for prefixing output)
    void SetIdentifier(const Util::String & id);
    /// get simple identifier of job
    const Util::String & GetIdentifier();
    /// get job guid
    const Util::Guid & GetGuid();
    /// set distributed flag
    void SetDistributedFlag(bool val);
    /// set whether job is running on a slave machine
    void SetRunningOnSlave(bool val);
    /// returns true if job is running on a slave machine
    bool IsRunningOnSlave();
    /// set shared dir path
    void SetSharedDirPath(const Util::String & path);
    /// set shared dir guid
    void SetSharedDirGuid(const Util::Guid & guid);
    /// validate the local prject path in the registry
    void ValidateProjectPath();
    /// skip the update process
    void SkipProjectDirectoryUpdate();
    /// get current svn revision of project directory
    Util::String GetCurrentRevision();
    /// update project directory
    void UpdateProjectDirectory();
    /// runs the application of the job
    virtual void RunJob();
    /// updates the jobs output stream and checks current state
    void Update();
    /// writes job as a job command to a given xmlwriter
    void GenerateJobCommand(const Ptr<IO::XmlWriter> & writer);
    /// setup job from given xml reader
    void SetupFromXml(const Ptr<IO::XmlReader> & reader);
    /// checks job setup and returns true if job is able to run on current environment
    virtual bool IsAbleToRun();
    /// Returns true if there is content in the output stream of the job
    virtual bool HasOutputContent();
    /// Get the content of the output stream
    virtual Util::String DequeueOutputContent();
    /// clones of this job (with a different guid)
    virtual Ptr<DistributedJob> Clone();
    /// copy the attributes of another job to itself, except the guid
    virtual void CopyFrom(const Ptr<DistributedJob> & job);

protected:
    /// called by GenerateJobCommand
    virtual void WriteXmlContent(const Ptr<IO::XmlWriter> & writer);
    /// called by SetupFromXml
    virtual void ReadXmlContent(const Ptr<IO::XmlReader> & reader);

    /// called by Update() when running job application
    virtual void UpdateJob();
    /// get the string representation of a jobState
    Util::String GetStateString();
    /// set state from string
    void SetStateFromString(const Util::String & str);
    /// write to the jobs output stream
    virtual void WriteOutput(Util::String text);

    DistributedJobState jobState;

private:
    /// called by Update() when running svn update
    void UpdateSVN();

    Util::String projectDirectory;
    Util::String svnPath;
    Util::String requiredRevision;
    bool svnReportedError;
    Ptr<IO::MemoryStream> svnErrStream;
    Ptr<IO::TextWriter> jobOutputWriter;

    ToolkitUtil::AppLauncher svnApplication;
    Util::Guid jobGuid;
    Util::String identifier;
    Util::String sharedDirPath;
    Util::Guid sharedDirGuid;
    bool distributedFlag;
    bool isRunningOnSlave;
};
//------------------------------------------------------------------------------
/**
    Set project directory for this job
*/
inline
void
DistributedJob::SetProjectDirectory(const Util::String & dir)
{
    n_assert(Ready == this->jobState);
    this->projectDirectory = dir;
}

//------------------------------------------------------------------------------
/**
    Get project directory for this job
*/
inline
const Util::String &
DistributedJob::GetProjectDirectory()
{
    return this->projectDirectory;
}
//------------------------------------------------------------------------------
/**
    Set svn path for this job
*/
inline
void
DistributedJob::SetSVNPath(const Util::String & path)
{
    n_assert(Ready == this->jobState);
    this->svnPath = path;
}

//------------------------------------------------------------------------------
/**
    Get svn path for this job
*/
inline
const Util::String &
DistributedJob::GetSVNPath()
{
    return this->svnPath;
}

//------------------------------------------------------------------------------
/**
    Set required revision id for this job
*/
inline
void
DistributedJob::SetRequiredRevision(const Util::String & revision)
{
    n_assert(Ready == this->jobState);
    this->requiredRevision = revision;
}

//------------------------------------------------------------------------------
/**
    Get required revision id for this job
*/
inline
const Util::String &
DistributedJob::GetRequiredRevision()
{
    return this->requiredRevision;
}

//------------------------------------------------------------------------------
/**
    Get state of this job
*/
inline
DistributedJob::DistributedJobState
DistributedJob::GetCurrentState()
{
    return this->jobState;
}
//------------------------------------------------------------------------------
/**
    Set simple identifier of this job. For prefixing output messages.
*/
inline
void
DistributedJob::SetIdentifier(const Util::String & id)
{
    this->identifier = id;
}
//------------------------------------------------------------------------------
/**
    Get simple identifier of this job. For prefixing output messages.
*/
inline
const Util::String &
DistributedJob::GetIdentifier()
{
    return this->identifier;
}
//------------------------------------------------------------------------------
/**
    Get job guid    
*/
inline
const Util::Guid &
DistributedJob::GetGuid()
{
    return this->jobGuid;
}

//------------------------------------------------------------------------------
/**
    Set distributed flag. If not set, job won't check against shareddir properties
*/
inline
void
DistributedJob::SetDistributedFlag(bool val)
{
    this->distributedFlag = val;
}

//------------------------------------------------------------------------------
/**
    Set whether the job is running on a slave machine. If this flag is
    set to true, the job may behave slightly different.
*/
inline
void
DistributedJob::SetRunningOnSlave(bool val)
{
    this->isRunningOnSlave = val;
}

//------------------------------------------------------------------------------
/**
    Returns true if job-flag was set that it is running on a
    slave machine.
*/
inline
bool
DistributedJob::IsRunningOnSlave()
{
    return this->isRunningOnSlave;
}

//------------------------------------------------------------------------------
/**
    Set path to the shared directory. The job won't start if that
    file does not exist.
*/
inline
void
DistributedJob::SetSharedDirPath(const Util::String & path)
{
    this->sharedDirPath = path;
}

//------------------------------------------------------------------------------
/**
    Set guid of shared directory. The job won't start if shared dir does not
    contain a subdirectory with that guid.
*/
inline
void
DistributedJob::SetSharedDirGuid(const Util::Guid & guid)
{
    this->sharedDirGuid = guid;
}

} // namespace DistributedTools