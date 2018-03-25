//------------------------------------------------------------------------------
//  distributedjob.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "distributedtools/distributedjobs/distributedjob.h"
#include "distributedtools/shareddircontrol.h"
#include "distributedtools/shareddircreator.h"
#include "io/ioserver.h"
#include "io/textreader.h"
#include "io/textwriter.h"
#include "system/nebulasettings.h"


using namespace Util;
using namespace IO;
using namespace ToolkitUtil;
using namespace System;
//------------------------------------------------------------------------------
namespace DistributedTools
{
__ImplementClass(DistributedTools::DistributedJob,'DTJB',Core::RefCounted);

//------------------------------------------------------------------------------
/**
    Constructor	
*/
DistributedJob::DistributedJob() :
    jobState(Ready),
    svnReportedError(false),
    distributedFlag(false),
    isRunningOnSlave(false)
{
    this->jobGuid.Generate();
}

//------------------------------------------------------------------------------
/**
    Destructor	
*/
DistributedJob::~DistributedJob()
{
    if (this->jobOutputWriter.isvalid())
    {
        if (this->jobOutputWriter->IsOpen())
        {
            this->jobOutputWriter->Close();
        }
        this->jobOutputWriter = nullptr;
    }
}
//------------------------------------------------------------------------------
/**
    Validates the current project path in the registry
*/
void
DistributedJob::ValidateProjectPath()
{
    n_assert(Ready == this->jobState);
    n_assert(JobError != this->jobState);

    String projDirectory;
    if (NebulaSettings::Exists("gscept","ToolkitShared", "workdir"))
    {
        projDirectory = NebulaSettings::ReadString("gscept","ToolkitShared", "workdir");
        if(projDirectory != this->projectDirectory)
        {
            if(!NebulaSettings::WriteString("gscept","ToolkitShared", "workdir",this->projectDirectory))
            {
                this->jobState = JobError;
                return;
            }
        }
        this->jobState = ValidProjectPath;
    }
    else
    {
        this->jobState = JobError;
    }
}
//------------------------------------------------------------------------------
/**
    Skip the svn update process. Use this only if you are 100% sure, that the
    job works on an up-to-date project directory
*/
void
DistributedJob::SkipProjectDirectoryUpdate()
{
    n_assert(JobError != this->jobState);
    n_assert(ValidProjectPath == this->jobState);
    this->jobState = UpToDate;
}

//------------------------------------------------------------------------------
/**
    Gets svn info of the current project dir and returns revision id
*/
String
DistributedJob::GetCurrentRevision()
{
    n_assert(this->svnPath.IsValid());
    n_assert(this->projectDirectory.IsValid());
    String out;

    AppLauncher svnApp;

    String svnArgs;
    svnArgs.Append("info");
    svnApp.SetArguments(svnArgs);

    URI svnUri;
    svnUri.Set(this->svnPath);
    svnApp.SetExecutable(svnUri);

    URI svnWorkUri;    
    svnWorkUri.Set(this->projectDirectory);
    svnApp.SetWorkingDirectory(svnWorkUri);

    Ptr<MemoryStream> stream = MemoryStream::Create();
    svnApp.SetStdoutCaptureStream(stream.upcast<Stream>());
    svnApp.LaunchWait();

    Ptr<TextReader> reader = TextReader::Create();
    reader->SetStream(stream.upcast<Stream>());
    reader->Open();
    const String searchFor = "Revision: ";
    while (!reader->Eof())
    {
        String line;
        line = reader->ReadLine();
        IndexT result = line.FindStringIndex(searchFor);
        if(result!=InvalidIndex)
        {
            out.Append(line.ExtractRange(result+searchFor.Length(),line.Length()-(result+searchFor.Length())));
            out.Trim(" \r\n");
            break;
        }
    }
    reader->Close();

    return out;
}

//------------------------------------------------------------------------------
/**
    Updates the project directory of this job
*/
void
DistributedJob::UpdateProjectDirectory()
{
	this->jobState = UpToDate;
	return;
    n_assert(JobError != this->jobState);
    n_assert(ValidProjectPath == this->jobState);
    n_assert(this->requiredRevision.IsValid());
    
    // skip the update process if required revision is equal with
    // the current svn revision.
    // this is much faster, than run the svn update on the same revision.
    if(this->GetCurrentRevision() == this->requiredRevision)
    {
        this->SkipProjectDirectoryUpdate();
        return;
    } 

    String svnArgs;
    svnArgs.Append("update --non-interactive --force -r ");
    svnArgs.Append(this->requiredRevision);
    this->svnApplication.SetArguments(svnArgs);
    
    URI svnUri;
    svnUri.Set(this->svnPath);
    this->svnApplication.SetExecutable(svnUri);
    
    URI svnWorkUri;    
    svnWorkUri.Set(this->projectDirectory);
    this->svnApplication.SetWorkingDirectory(svnWorkUri);

    this->svnErrStream = MemoryStream::Create();
    this->svnApplication.SetStderrCaptureStream(svnErrStream.upcast<Stream>());
    
    if(this->svnApplication.Launch())
    {
        this->jobState = Updating;
    }
    else
    {
        this->jobState = JobError;
    }

}

//------------------------------------------------------------------------------
/**
    Runs the application of this job
*/
void
DistributedJob::RunJob()
{
    n_assert(JobError != this->jobState);
    n_assert(UpToDate == this->jobState);
    
    this->jobState = Working;

    // do some stuff
}

//------------------------------------------------------------------------------
/**
    Update the job. Checks if current action has finished.
    Updates output stream.
*/
void
DistributedJob::Update()
{
    n_assert(JobError != this->jobState);
    n_assert(Updating == this->jobState || Working == this->jobState);

    if(Updating == this->jobState)
    {
        this->UpdateSVN();
    }
    else if(Working == this->jobState)
    {
        this->UpdateJob();
    }
}
//------------------------------------------------------------------------------
/**
    Generate a xml command from this job	
*/
void
DistributedJob::GenerateJobCommand(const Ptr<IO::XmlWriter> & writer)
{
    writer->BeginNode("JobCommand");
    this->WriteXmlContent(writer);
    writer->EndNode();
}

//------------------------------------------------------------------------------
/**
    reads xmlnode from given reader	and setup itself
*/
void
DistributedJob::SetupFromXml(const Ptr<IO::XmlReader> & reader)
{
    if (reader->HasNode("JobCommand"))
    {
        reader->SetToFirstChild("JobCommand");
        this->ReadXmlContent(reader);
        reader->SetToParent();
    }
}

//------------------------------------------------------------------------------
/**
    writes basic attributes to xml stream. derive from this for further content
*/
void
DistributedJob::WriteXmlContent(const Ptr<IO::XmlWriter> & writer)
{
    writer->SetString("guid",this->jobGuid.AsString());
    writer->SetString("state",this->GetStateString());
    writer->SetString("identifier",this->identifier);
    writer->SetString("projectdir",this->projectDirectory);
    writer->SetBool("onslavemachine",this->isRunningOnSlave);
    if(this->distributedFlag)
    {
        writer->BeginNode("Distributed");
            writer->SetString("shareddirpath",this->sharedDirPath);
            writer->SetString("shareddirguid",this->sharedDirGuid.AsString());
        writer->EndNode();
    }
    writer->BeginNode("SVNSettings");
        writer->SetString("path",this->svnPath);
        writer->SetString("revision",this->requiredRevision);
    writer->EndNode();
}
//------------------------------------------------------------------------------
/**
    reads basic attributes from xml stream. derive from this for further content
*/
void
DistributedJob::ReadXmlContent(const Ptr<IO::XmlReader> & reader)
{
    if (reader->HasAttr("guid"))
    {
        this->jobGuid = Guid::FromString(reader->GetString("guid"));
    }
    if (reader->HasAttr("state"))
    {
        this->SetStateFromString(reader->GetString("state"));
    }
    if (reader->HasAttr("identifier"))
    {
        this->identifier = reader->GetString("identifier");
    }
    if (reader->HasAttr("projectdir"))
    {
        this->projectDirectory = reader->GetString("projectdir");
    }
    if (reader->HasAttr("onslavemachine"))
    {
        this->isRunningOnSlave = reader->GetBool("onslavemachine");
    }
    this->distributedFlag = reader->HasNode("Distributed");
    if(reader->HasNode("Distributed"))
    {
        reader->SetToFirstChild("Distributed");
        if (reader->HasAttr("shareddirpath"))
        {
            this->sharedDirPath = reader->GetString("shareddirpath");
        }
        if (reader->HasAttr("shareddirguid"))
        {
            this->sharedDirGuid = Guid::FromString(reader->GetString("shareddirguid"));
        }
        reader->SetToParent();
    }
    if(reader->HasNode("SVNSettings"))
    {
        reader->SetToFirstChild("SVNSettings");
        if(reader->HasAttr("path"))
        {
            this->svnPath = reader->GetString("path");
        }
        if(reader->HasAttr("revision"))
        {
            this->requiredRevision = reader->GetString("revision");
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
DistributedJob::IsAbleToRun()
{
    bool success = true;
    
    // check svn path
    URI svnUri;
    svnUri.Set(this->svnPath);
    if(!IoServer::Instance()->FileExists(svnUri))
    {
        success = false;
    }
    
    // check project path
    URI projUri;
    projUri.Set(this->projectDirectory);
    if(!IoServer::Instance()->DirectoryExists(projUri))
    {
        success = false;
    }

    // check valid output path
    if(this->distributedFlag)
    {
        Ptr<SharedDirControl> sharedDir = SharedDirCreator::Instance()->CreateSharedControlObject(this->sharedDirPath);
        success = sharedDir->ContainsGuidSubDir(this->sharedDirGuid);
        sharedDir = nullptr;
    }

    return success;
}

//------------------------------------------------------------------------------
/**
    Check if svn has finished and updates output stream
*/
void
DistributedJob::UpdateSVN()
{
	this->jobState = UpToDate;
	return;
    n_assert(JobError != this->jobState);
    n_assert(Updating == this->jobState);
    if(!this->svnApplication.IsRunning())
    {
        // check if svn reports an error through the error stream.
        // if an error happens, the job can not continue...
        if (this->svnErrStream->GetSize() > 0)
        {
            Ptr<TextReader> reader = TextReader::Create();
            reader->SetStream(this->svnErrStream.upcast<Stream>());
            reader->Open();
            n_printf("[Job #%s] ERROR WHILE SVN UPDATE\nsvn output:\n%s\n",this->identifier.AsCharPtr(),reader->ReadAll().AsCharPtr());
            reader->Close();
            reader = nullptr;
            this->jobState = JobError;
        }
        else
        {
            this->jobState = UpToDate;
        }
        // svn error stream no longer needed
        this->svnErrStream = nullptr;
    }
}
//------------------------------------------------------------------------------
/**
    Check if job application has finished and updates output stream
*/
void
DistributedJob::UpdateJob()
{
    n_assert(JobError != this->jobState);
    n_assert(Working == this->jobState);

    this->jobState = Finished;
}

//------------------------------------------------------------------------------
/**
    Writes a given string to the jobs output stream
*/
void
DistributedJob::WriteOutput(Util::String text)
{
    // create an output stream, if it does not exist already
    if (!this->jobOutputWriter.isvalid())
    {
        this->jobOutputWriter = TextWriter::Create();
        this->jobOutputWriter->SetStream(MemoryStream::Create());
    }
        
    if (!this->jobOutputWriter->IsOpen())
    {
        this->jobOutputWriter->Open();
    }
    if (this->jobOutputWriter->IsOpen())
    {
        this->jobOutputWriter->WriteString(text);
    }
}

//------------------------------------------------------------------------------
/**
    Returns true, if the job has some output	
*/
bool
DistributedJob::HasOutputContent()
{
    if (this->jobOutputWriter.isvalid())
    {
        if (this->jobOutputWriter->HasStream())
        {
            if (this->jobOutputWriter->GetStream()->GetSize() > 0)
            {
                return true;
            }
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Returns the content of the jobs output	
*/
Util::String
DistributedJob::DequeueOutputContent()
{
    String out;
    if (this->HasOutputContent())
    {
        n_assert(this->jobOutputWriter.isvalid());
        
        // close output writer
        this->jobOutputWriter->Close();
        
        Ptr<TextReader> reader = TextReader::Create();
        reader->SetStream(this->jobOutputWriter->GetStream());
        if (reader->Open())
        {
            out = reader->ReadAll();
            reader->Close();
        }
        reader = nullptr;
        this->jobOutputWriter = nullptr;
    }
    return out;
}

//------------------------------------------------------------------------------
/**
    clones of this job (with a different guid)
*/
Ptr<DistributedJob>
DistributedJob::Clone()
{
    Ptr<DistributedJob> res = DistributedJob::Create();
    res->SetIdentifier(this->identifier);
    res->SetDistributedFlag(this->distributedFlag);
    res->SetSharedDirPath(this->sharedDirPath);
    res->SetSharedDirGuid(this->sharedDirGuid);
    res->SetProjectDirectory(this->projectDirectory);
    res->SetRequiredRevision(this->requiredRevision);
    res->SetStateFromString(this->GetStateString());
    res->SetSVNPath(this->svnPath);

    res->svnReportedError = this->svnReportedError;
    res->svnErrStream = this->svnErrStream;
    res->svnApplication = this->svnApplication;

    return res;
}
//------------------------------------------------------------------------------
/**
    Copy the attributes from another job to itself, except the guid	
*/
void
DistributedJob::CopyFrom(const Ptr<DistributedJob> & job)
{
    this->SetIdentifier(job->identifier);
    this->SetDistributedFlag(job->distributedFlag);
    this->SetSharedDirPath(job->sharedDirPath);
    this->SetSharedDirGuid(job->sharedDirGuid);
    this->SetProjectDirectory(job->projectDirectory);
    this->SetRequiredRevision(job->requiredRevision);
    this->SetStateFromString(job->GetStateString());
    this->SetSVNPath(job->svnPath);

    this->svnReportedError = job->svnReportedError;
    this->svnErrStream = job->svnErrStream;
    this->svnApplication = job->svnApplication;
}

//------------------------------------------------------------------------------
/**
    returns a string representation of teh current job state	
*/
String
DistributedJob::GetStateString()
{
    String output = "JobError";
    if(Ready == this->jobState)
    {
        return "Ready";
    }
    else if(Updating == this->jobState)
    {
        return "Updating";
    }
    else if(UpToDate == this->jobState)
    {
        return "UpToDate";
    }
    else if(Working == this->jobState)
    {
        return "Working";
    }
    else if(Finished == this->jobState)
    {
        return "Finished";
    }
    else if(JobError == this->jobState)
    {
        return "JobError";
    }
    return output;
}

//------------------------------------------------------------------------------
/**
    Sets the jobs state from a given string	
*/
void
DistributedJob::SetStateFromString(const Util::String & str)
{
    if("Ready" == str)
    {
        this->jobState = Ready;
    }
    else if("Updating" == str)
    {
        this->jobState = Updating;
    }
    else if("UpToDate" == str)
    {
        this->jobState = UpToDate;
    }
    else if("Working" == str)
    {
        this->jobState = Working;
    }
    else if("Finished" == str)
    {
        this->jobState = Finished;
    }
    else 
    {
        this->jobState = JobError;
    }
    
}
} // namespace DistributedTools