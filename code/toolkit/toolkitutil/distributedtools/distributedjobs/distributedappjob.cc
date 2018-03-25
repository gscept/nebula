//------------------------------------------------------------------------------
//  distributedappjob.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "distributedtools/distributedjobs/distributedappjob.h"
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
__ImplementClass(DistributedAppJob,'DTAJ',DistributedTools::DistributedJob);

//------------------------------------------------------------------------------
/**
    Constructor	
*/
DistributedAppJob::DistributedAppJob() :
    appReportedError(false)
{

}

//------------------------------------------------------------------------------
/**
    Destructor	
*/
DistributedAppJob::~DistributedAppJob()
{
}

//------------------------------------------------------------------------------
/**
    Run the job. In this case start an application
*/
void
DistributedAppJob::RunJob()
{
    n_assert(JobError != this->jobState);
    n_assert(UpToDate == this->jobState);
    
    // create a temporary list file on the jobs host system
    this->localListPath.Set("temp:distributedtools/"+this->GetGuid().AsString()+".txt");   
    if(!IoServer::Instance()->DirectoryExists(this->localListPath.LocalPath().ExtractDirName()))
    {
        IoServer::Instance()->CreateDirectory(this->localListPath.LocalPath().ExtractDirName());
    }
    
    // write temporary list file
    Ptr<TextWriter> writer = TextWriter::Create();
    writer->SetStream(IoServer::Instance()->CreateStream(this->localListPath));
    writer->Open();
    IndexT fileIndex;
    for (fileIndex = 0; fileIndex < this->fileList.Size(); fileIndex++)
    {
        writer->WriteString(this->fileList[fileIndex]);   
        writer->WriteString("\r\n");
    }
    IndexT dirIndex;
    for (dirIndex = 0; dirIndex < this->dirList.Size(); dirIndex++)
    {
        Array<String> dirFiles = IoServer::Instance()->ListFiles(this->dirList[dirIndex], "*");
        for (fileIndex = 0; fileIndex < dirFiles.Size(); fileIndex++)
        {
            String path;
            path.Format("%s/%s", this->dirList[dirIndex].AsCharPtr(), dirFiles[fileIndex].AsCharPtr());
            writer->WriteString(path);
            writer->WriteString("\r\n");
        }
    }

    writer->Close();
    writer = nullptr;

    String jobArgs;
    jobArgs.Append(this->arguments);
    jobArgs.Append(" -listfile ");
    jobArgs.Append(this->localListPath.LocalPath());

    // if the job is runnign on a local machine,
    // the application should know it, to behave different.
    if (this->IsRunningOnSlave())
    {
        jobArgs.Append(" -slave");
    }

    this->jobApplication.SetArguments(jobArgs);
    
    URI appUri = URI(this->applicationPath);
    this->jobApplication.SetExecutable(appUri.LocalPath());
    this->jobApplication.SetWorkingDirectory(appUri.LocalPath().ExtractDirName());
    
    this->appOutStream = MemoryStream::Create();
    this->jobApplication.SetStdoutCaptureStream(appOutStream.upcast<Stream>());
    
    if(this->jobApplication.Launch())
    {
        this->jobState = Working;
    }
    else
    {
        this->jobState = JobError;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DistributedAppJob::WriteXmlContent(const Ptr<IO::XmlWriter> & writer)
{
    // write basic attributes
    DistributedJob::WriteXmlContent(writer);
    // write app specific attributes
    writer->BeginNode("AppSettings");
        writer->SetString("path",this->applicationPath);
        writer->SetString("arguments",this->arguments);
    writer->EndNode();
    writer->BeginNode("FileList");
    IndexT file;
    for(file = 0; file < this->fileList.Size(); file++)
    {
    	writer->BeginNode("File");
            writer->SetString("path",this->fileList[file]);
        writer->EndNode();
    }
    writer->EndNode();
    writer->BeginNode("DirList");
    IndexT dir;
    for(dir = 0; dir < this->dirList.Size(); dir++)
    {
    	writer->BeginNode("Dir");
            writer->SetString("path",this->dirList[dir]);
        writer->EndNode();
    }
    writer->EndNode();
}
//------------------------------------------------------------------------------
/**
*/
void
DistributedAppJob::ReadXmlContent(const Ptr<IO::XmlReader> & reader)
{
    // read basic attributes
    DistributedJob::ReadXmlContent(reader);
    // read app specific attributes
    if(reader->HasNode("AppSettings"))
    {
        reader->SetToFirstChild("AppSettings");
        if(reader->HasAttr("path"))
        {
            this->applicationPath = reader->GetString("path");
        }
        if(reader->HasAttr("arguments"))
        {
            this->arguments = reader->GetString("arguments");
        }
        reader->SetToParent();
    }
    if(reader->HasNode("FileList"))
    {
        reader->SetToFirstChild("FileList");
        if(reader->HasNode("File"))
        {
            reader->SetToFirstChild("File");
            do 
            {
                if(reader->HasAttr("path"))
                {
                    this->fileList.Append(reader->GetString("path"));
                }
            } while (reader->SetToNextChild("File"));
        }
        reader->SetToParent();
    }
    if(reader->HasNode("DirList"))
    {
        reader->SetToFirstChild("DirList");
        if(reader->HasNode("Dir"))
        {
            reader->SetToFirstChild("Dir");
            do 
            {
                if(reader->HasAttr("path"))
                {
                    this->dirList.Append(reader->GetString("path"));
                }
            } while (reader->SetToNextChild("Dir"));
        }
        reader->SetToParent();
    }
}

//------------------------------------------------------------------------------
/**
    Do some file checks to indicate if job is able to run under the
    current environment.
*/
bool
DistributedAppJob::IsAbleToRun()
{
    bool success = DistributedJob::IsAbleToRun();
    
    // check application path
    if(!IoServer::Instance()->FileExists(this->applicationPath))
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
DistributedAppJob::UpdateJob()
{
    n_assert(JobError != this->jobState);
    n_assert(Working == this->jobState);

    if(!this->jobApplication.IsRunning())
    {
        // check if a list file was created and remove it from disk
        if(IoServer::Instance()->FileExists(this->localListPath))
        {
            IoServer::Instance()->DeleteFile(this->localListPath);
        }
        // print output of job
        Ptr<TextReader> reader = TextReader::Create();
        reader->SetStream(this->appOutStream.upcast<Stream>());
        reader->Open();
        String line;
        while (!reader->Eof())
        {
            line.Format("[#%s] %s\n",this->GetIdentifier().AsCharPtr(),reader->ReadLine().AsCharPtr());
            this->WriteOutput(line);
        }
        reader->Close();
        reader = nullptr;
        this->appOutStream = nullptr;

        this->jobState = Finished;
    }
    else
    {
        // flush output stream if it gets to big
        if(this->appOutStream->GetSize() > 2048)
        {
            this->appOutStream->Close();
            this->appOutStream->SetAccessMode(Stream::ReadAccess);
            // print output of job
            Ptr<TextReader> reader = TextReader::Create();
            reader->SetStream(this->appOutStream.upcast<Stream>());
            reader->Open();
            String line;
            while (!reader->Eof())
            {
                line.Format("[#%s] %s\n",this->GetIdentifier().AsCharPtr(),reader->ReadLine().AsCharPtr());
                this->WriteOutput(line);
            }
            reader->Close();
            reader = nullptr;
            
            this->appOutStream->SetSize(0);
            this->appOutStream->SetAccessMode(Stream::WriteAccess);
            this->appOutStream->Open();
        }
    }

}

//------------------------------------------------------------------------------
/**
    clones of this job (with a different guid)
*/
Ptr<DistributedJob>
DistributedAppJob::Clone()
{
    Ptr<DistributedAppJob> res = DistributedAppJob::Create();
    res->CopyFrom(this);
    
    res->SetApplicationPath(this->applicationPath);
    res->SetArguments(this->arguments);
    res->SetFileList(this->fileList);
    res->SetDirList(this->dirList);

    res->appReportedError = this->appReportedError;
    res->appOutStream = this->appOutStream;
    res->jobApplication = this->jobApplication;
    res->localListPath = this->localListPath;

    return res.upcast<DistributedJob>();
}

//------------------------------------------------------------------------------
/**
	Copy the attributes from another job to itself, except the guid	
*/
void
DistributedAppJob::CopyFrom(const Ptr<DistributedAppJob> & job)
{
    DistributedJob::CopyFrom(job.upcast<DistributedJob>());
    this->SetApplicationPath(job->applicationPath);
    this->SetArguments(job->arguments);
    this->SetFileList(job->fileList);
    this->SetDirList(job->dirList);

    this->appReportedError = job->appReportedError;
    this->appOutStream = job->appOutStream;
    this->jobApplication = job->jobApplication;
    this->localListPath = job->localListPath;
}

} // namespace DistributedTools