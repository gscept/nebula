//------------------------------------------------------------------------------
//  distributedcreateemptyfilejob.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "distributedtools/distributedjobs/distributedcreateemptyfilejob.h"
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
__ImplementClass(DistributedCreateEmptyFileJob,'DCFJ',DistributedJob);

//------------------------------------------------------------------------------
/**
    Constructor	
*/
DistributedCreateEmptyFileJob::DistributedCreateEmptyFileJob()
{
}

//------------------------------------------------------------------------------
/**
    Destructor	
*/
DistributedCreateEmptyFileJob::~DistributedCreateEmptyFileJob()
{
}

//------------------------------------------------------------------------------
/**
    Runs the application of this job
*/
void
DistributedCreateEmptyFileJob::RunJob()
{
    n_assert(JobError != this->jobState);
    n_assert(UpToDate == this->jobState);
    
    this->jobState = Working;

    Ptr<IO::Stream> stream = IoServer::Instance()->CreateStream(this->filePath);
    stream->SetAccessMode(IO::Stream::ReadWriteAccess);
    if(stream->Open())
    {
        stream->Close();
    }
}

//------------------------------------------------------------------------------
/**
    writes basic attributes to xml stream. derive from this for further content
*/
void
DistributedCreateEmptyFileJob::WriteXmlContent(const Ptr<IO::XmlWriter> & writer)
{
    // write basic attributes
    DistributedJob::WriteXmlContent(writer);
    // write delete directory specific attributes
    writer->SetString("filepath", this->filePath.LocalPath());

}
//------------------------------------------------------------------------------
/**
    reads basic attributes from xml stream. derive from this for further content
*/
void
DistributedCreateEmptyFileJob::ReadXmlContent(const Ptr<IO::XmlReader> & reader)
{
    // read basic attributes
    DistributedJob::ReadXmlContent(reader);
    // read delete directory specific attributes
    if (reader->HasAttr("filepath"))
    {
        this->filePath = reader->GetString("filepath");
    }
}

//------------------------------------------------------------------------------
/**
    Do some file checks to indicate if job is able to run under the
    current environment.
*/
bool
DistributedCreateEmptyFileJob::IsAbleToRun()
{
    bool success = DistributedJob::IsAbleToRun();
    return success;
}

//------------------------------------------------------------------------------
/**
    Check if job application has finished and updates output stream
*/
void
DistributedCreateEmptyFileJob::UpdateJob()
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
DistributedCreateEmptyFileJob::Clone()
{
    Ptr<DistributedCreateEmptyFileJob> res = DistributedCreateEmptyFileJob::Create();
    res->CopyFrom(this);

    res->SetFilePath(this->filePath);

    return res.upcast<DistributedJob>();
}
//------------------------------------------------------------------------------
/**
    copy the attributes of another job to itself, except the guid	
*/
void
DistributedCreateEmptyFileJob::CopyFrom(const Ptr<DistributedCreateEmptyFileJob> & job)
{
    DistributedJob::CopyFrom(job.upcast<DistributedJob>());
    this->SetFilePath(job->filePath);
}

} // namespace DistributedTools