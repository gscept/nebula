#pragma once
//------------------------------------------------------------------------------
/**
    @class DistributedTools::DistributedCreateEmptyFileJob

    Creates an empty file at a specifies path.

    (C) 2010 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "distributedtools/distributedjobs/distributedjob.h"

//------------------------------------------------------------------------------
namespace DistributedTools
{
class DistributedCreateEmptyFileJob : public DistributedJob
{
__DeclareClass(DistributedTools::DistributedCreateEmptyFileJob);
public:
    /// Constructor
    DistributedCreateEmptyFileJob();
    /// Distructor
    virtual ~DistributedCreateEmptyFileJob();
    
    /// set file path
    void SetFilePath(const IO::URI & path);
    /// get file path
    const IO::URI & GetFilePath();
    /// runs the application of the job
    void RunJob();
    /// checks job setup and returns true if job is able to run on current environment
    bool IsAbleToRun();
    /// clones of this job (with a different guid)
    Ptr<DistributedJob> Clone();
    /// copy the attributes of another job to itself, except the guid
    virtual void CopyFrom(const Ptr<DistributedCreateEmptyFileJob> & job);

private:
    /// called by GenerateJobCommand
    void WriteXmlContent(const Ptr<IO::XmlWriter> & writer);
    /// called by SetupFromXml
    void ReadXmlContent(const Ptr<IO::XmlReader> & reader);
    /// called by Update() when running job application
    void UpdateJob();

    IO::URI filePath;
};

//------------------------------------------------------------------------------
/**
    Set file path for this job
*/
inline
void
DistributedCreateEmptyFileJob::SetFilePath(const IO::URI & path)
{
    n_assert(Ready == this->jobState);
    this->filePath = path;
}

//------------------------------------------------------------------------------
/**
    Get file path for this job
*/
inline
const IO::URI &
DistributedCreateEmptyFileJob::GetFilePath()
{
    return this->filePath;
}

} // namespace DistributedTools