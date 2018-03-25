//------------------------------------------------------------------------------
//  distributeddeletedirectorycontentjob.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "distributedtools/distributedjobs/distributeddeletedirectorycontentjob.h"
#include "io/ioserver.h"
#include "io/textreader.h"
#include "io/textwriter.h"

using namespace Util;
using namespace IO;
using namespace ToolkitUtil;
using namespace System;
//------------------------------------------------------------------------------
namespace DistributedTools
{
__ImplementClass(DistributedDeleteDirectoryContentJob,'DDCJ',DistributedJob);

//------------------------------------------------------------------------------
/**
    Constructor	
*/
DistributedDeleteDirectoryContentJob::DistributedDeleteDirectoryContentJob()
{

}

//------------------------------------------------------------------------------
/**
    Destructor	
*/
DistributedDeleteDirectoryContentJob::~DistributedDeleteDirectoryContentJob()
{
}

//------------------------------------------------------------------------------
/**
    Runs the application of this job
*/
void
DistributedDeleteDirectoryContentJob::RunJob()
{
    n_assert(JobError != this->jobState);
    n_assert(UpToDate == this->jobState);
    
    this->jobState = Working;

    // get all files from directory
    Array<String> files = IoServer::Instance()->ListFiles(this->delDirPath.LocalPath(), "*");
    // delete each file
    IndexT fileIndex;
    for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
    {
        String file;
        file.Format("%s/%s", this->delDirPath.LocalPath().AsCharPtr(), files[fileIndex].AsCharPtr());
        IoServer::Instance()->DeleteFile(file);
    }

    
}

//------------------------------------------------------------------------------
/**
    writes basic attributes to xml stream. derive from this for further content
*/
void
DistributedDeleteDirectoryContentJob::WriteXmlContent(const Ptr<IO::XmlWriter> & writer)
{
    // write basic attributes
    DistributedJob::WriteXmlContent(writer);
    // write delete directory specific attributes
    writer->SetString("deldirpath", this->delDirPath.LocalPath());

}
//------------------------------------------------------------------------------
/**
    reads basic attributes from xml stream. derive from this for further content
*/
void
DistributedDeleteDirectoryContentJob::ReadXmlContent(const Ptr<IO::XmlReader> & reader)
{
    // read basic attributes
    DistributedJob::ReadXmlContent(reader);
    // read delete directory specific attributes
    if (reader->HasAttr("deldirpath"))
    {
        this->delDirPath = reader->GetString("deldirpath");
    }
}

//------------------------------------------------------------------------------
/**
    Do some file checks to indicate if job is able to run under the
    current environment.
*/
bool
DistributedDeleteDirectoryContentJob::IsAbleToRun()
{
    bool success = DistributedJob::IsAbleToRun();
    // check directory path
    if(!this->delDirPath.IsValid())
    {
        success = false;
    }
    if(!IoServer::Instance()->DirectoryExists(this->delDirPath))
    {
        success = false;
    }
    return success;
}

//------------------------------------------------------------------------------
/**
    Check if job application has finished and updates output stream
*/
void
DistributedDeleteDirectoryContentJob::UpdateJob()
{
    n_assert(JobError != this->jobState);
    n_assert(Working == this->jobState);

    this->jobState = Finished;
}

//------------------------------------------------------------------------------
/**
    clones of this job (with a different guid)
*/
Ptr<DistributedJob>
DistributedDeleteDirectoryContentJob::Clone()
{
    Ptr<DistributedDeleteDirectoryContentJob> res = DistributedDeleteDirectoryContentJob::Create();
    res->CopyFrom(this);

    res->SetDelDirectoryPath(this->delDirPath);

    return res.upcast<DistributedJob>();
}
//------------------------------------------------------------------------------
/**
    copy the attributes of another job to itself, except the guid	
*/
void
DistributedDeleteDirectoryContentJob::CopyFrom(const Ptr<DistributedDeleteDirectoryContentJob> & job)
{
    DistributedJob::CopyFrom(job.upcast<DistributedJob>());
    this->SetDelDirectoryPath(job->delDirPath);
}

} // namespace DistributedTools