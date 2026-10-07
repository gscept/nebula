#pragma once
//------------------------------------------------------------------------------
/**
    @class DistributedTools::DistributedDeleteDirectoryContentJob

    @TODO comment

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "distributedtools/distributedjobs/distributedjob.h"

//------------------------------------------------------------------------------
namespace DistributedTools
{
class DistributedDeleteDirectoryContentJob : public DistributedJob
{
__DeclareClass(DistributedTools::DistributedDeleteDirectoryContentJob);
public:
    /// Constructor
    DistributedDeleteDirectoryContentJob();
    /// Distructor
    virtual ~DistributedDeleteDirectoryContentJob();
    
    /// set delete directory path
    void SetDelDirectoryPath(const IO::URI & path);
    /// get delete directory path
    const IO::URI & GetDelDirectoryPath();
    /// runs the application of the job
    void RunJob();
    /// checks job setup and returns true if job is able to run on current environment
    bool IsAbleToRun();
    /// clones of this job (with a different guid)
    Ptr<DistributedJob> Clone();
    /// copy the attributes of another job to itself, except the guid
    virtual void CopyFrom(const Ptr<DistributedDeleteDirectoryContentJob> & job);

private:
    /// called by GenerateJobCommand
    void WriteXmlContent(const Ptr<IO::XmlWriter> & writer);
    /// called by SetupFromXml
    void ReadXmlContent(const Ptr<IO::XmlReader> & reader);
    /// called by Update() when running job application
    void UpdateJob();

    IO::URI delDirPath;
};

//------------------------------------------------------------------------------
/**
    Set delete directory path for this job
*/
inline
void
DistributedDeleteDirectoryContentJob::SetDelDirectoryPath(const IO::URI & path)
{
    n_assert(Ready == this->jobState);
    this->delDirPath = path;
}

//------------------------------------------------------------------------------
/**
    Get delete directory path for this job
*/
inline
const IO::URI &
DistributedDeleteDirectoryContentJob::GetDelDirectoryPath()
{
    return this->delDirPath;
}

} // namespace DistributedTools