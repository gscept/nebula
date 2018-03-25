//------------------------------------------------------------------------------
//  distributedtoolkitapp.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "distributedtools/distributedtoolkitapp.h"
#include "distributedtools/distributedjobs/distributedappjob.h"
#include "distributedtools/distributedjobscheduler.h"
#include "distributedtools/remotejobservice.h"
#include "io/ioserver.h"
#include "io/textreader.h"
#include "io/uri.h"
#include "io/memorystream.h"
#include "timing/timer.h"
#include "system/nebulasettings.h"

using namespace Util;
using namespace IO;
using namespace System;
//------------------------------------------------------------------------------
namespace DistributedTools
{
//------------------------------------------------------------------------------
/**
    Constructor	
*/
DistributedToolkitApp::DistributedToolkitApp() :
    masterMode(false),
    jobsArg(1),
    processorsArg(1),
    slaveArg(false),
    forceArg(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
    Destructor	
*/
DistributedToolkitApp::~DistributedToolkitApp()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
DistributedToolkitApp::Open()
{
    if (!ToolkitApp::Open())
    {
        return false;
    }
    this->sharedDirCreator = SharedDirCreator::Create();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
DistributedToolkitApp::Close()
{
    // remove all destination directory content, if the
    // application was in slave mode on a remote computer
    if (this->distributedArg && this->slaveArg)
    {
        this->DeleteDirectoryContent(URI("dst:"));
        IoServer::Instance()->DeleteDirectory(URI("dst:"));
    }
    // dispose shared dir creator
    this->sharedDirCreator = nullptr;
    // if shared dir control is valid , dispose it
    if(this->sharedDirControl.isvalid())
    {
        this->sharedDirControl = nullptr;
    }
    ToolkitApp::Close();
}

//------------------------------------------------------------------------------
/**
	Parse the commandline args and setup application parameters
*/
bool
DistributedToolkitApp::ParseCmdLineArgs()
{
    if(!ToolkitApp::ParseCmdLineArgs())
    {
        return false;
    }      
    
    // setup the project info object
    if (!this->SetupProjectInfo())
    {
        return false;
    }
    this->sharedDirCreator->SetProjectInfo(this->projectInfo);

    if(this->args.HasArg("-jobs"))
    {
        // if a jobs arg was given, this class switch to master mode
        this->masterMode = true;
        this->jobsArg = this->args.GetInt("-jobs",1);
    }
    this->processorsArg  = this->args.GetInt("-processors",1);
    this->projectdirArg  = this->args.GetString("-projectdir",URI("proj:").LocalPath());
    this->dirArg         = this->args.GetString("-dir","");
    this->fileArg        = this->args.GetString("-file","");
    this->listfileArg    = this->args.GetString("-listfile","");
    this->distributedArg = this->args.GetBoolFlag("-distributed");
    this->shareddirArg   = this->args.GetString("-shareddir","");
    this->slaveArg       = this->args.GetBoolFlag("-slave");
    this->forceArg       = this->args.GetBoolFlag("-force");
    this->verboseFlag    = this->args.GetBoolFlag("-verbose");

    // if a list arg is given, the dir and file args are invalid
    if(
        this->listfileArg.IsValid() &&
        (this->dirArg.IsValid() || this->fileArg.IsValid()))
    {
        n_printf("no dir or file arg allowed, when listfile is given\n\n");
        this->ShowHelp();
        return false;
    }
    // if a file arg is given, a dir arg have to exist
    if(this->fileArg.IsValid() && !this->dirArg.IsValid())
    {
        n_printf("missing file argument\n\n");
        this->ShowHelp();
        return false;
    }
    // if the distributed arg is set, we need a shared directory
    if(this->args.HasArg("-distributed") && !this->args.HasArg("-shareddir"))
    {
        n_printf("missing shareddir argument\n\n");
        this->ShowHelp();
        return false;
    }
    // if the slave and distributed arg is set, we need also a shared guid
    if(
        this->args.HasArg("-slave") &&
        this->args.HasArg("-distributed") &&
        !this->args.HasArg("-sharedguid"))
    {
        n_printf("missing sharedguid argument\n\n");
        this->ShowHelp();
        return false;
    }
    // the slave arg should never be used together with the jobs arg
    if(this->args.HasArg("-slave") && this->args.HasArg("-jobs"))
    {
        n_printf("no jobs argument allowed for slave\n\n");
        this->ShowHelp();
        return false;
    }

    if(
        this->args.HasArg("-jobs") &&
        this->args.HasArg("-distributed") &&
        !this->args.HasArg("-slave"))
    {
        // create sharedDirControl for master
        Guid guid;
        guid.Generate();
        this->sharedDirControl =
            this->sharedDirCreator->CreateSharedControlObject(this->shareddirArg, guid);
    }
    if(
        this->slaveArg &&
        this->distributedArg &&
        this->args.HasArg("-sharedguid")
        )
    {
        Guid guid = Guid::FromString(this->args.GetString("-sharedguid",""));
        // create sharedDirControl for slave. Shared dir has to have a sub
        // directory which is named after the same guid from sharedguid arg
        this->sharedDirControl =
            this->sharedDirCreator->CreateSharedControlObject(this->shareddirArg, guid);
        // if that's not the case, the job should have failed way before
        n_assert(this->sharedDirControl->ContainsGuidSubDir(guid));
        this->sharedDirControl->SetUp();

        // re-assign destination dir if in slave mode
        n_assert(this->projectInfo.HasAttr("SlaveDstDir"));
        Guid subFolderGuid;
        subFolderGuid.Generate();
        URI slaveDst(this->projectInfo.GetPathAttr("SlaveDstDir"));
        slaveDst.AppendLocalPath(subFolderGuid.AsString());
        AssignRegistry::Instance()->SetAssign(Assign("dst",slaveDst.LocalPath()));
        
        // Create the new destination dir. It is cleared and removed on application close.
        int tries = 0;
        while (!IoServer::Instance()->DirectoryExists(URI("dst:")))
        {
            IoServer::Instance()->CreateDirectory(URI("dst:"));
            if (!IoServer::Instance()->DirectoryExists(URI("dst:")))
            {
                tries++;
                if (tries > 3)
                {
                    // at least we need an output dir...
                    Console::Instance()->Error(
                        "Could not create destination directory:\n%s\n",
                        URI("dst:").LocalPath().AsCharPtr());
                }
                else
                {
                    Console::Instance()->Warning(
                        "Could not create destination directory:\n%s\nRetry in 3 seconds...\n",
                        URI("dst:").LocalPath().AsCharPtr());
                    n_sleep(3);
                }
            }
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Get a string, that contains a description of the standard command line arguments for
    a distributed toolkit application
*/
String
DistributedToolkitApp::GetArgumentDescriptionString()
{
    String output;
    output.Format(
        "-help        -- display this help\n"
        "-waitforkey  -- wait for key press after finished\n"
        "-projectdir  -- absolute path to the project directory\n"
        "-dir         -- path to a single directory the tool should process\n"
        "-file        -- name of a single file the tool should process [requires -dir]\n"
        "-listfile    -- path to a text file with lines of files to process\n"
        "                (if -listfile arg is given, -dir and -file args are invalid)\n"
        "-jobs        -- how many parallel jobs should be used\n"
        "-processors  -- number of local used processors\n"
        "-shareddir   -- absolute path to shared output directory\n"
        "                (should be a shared network resource)\n"
        "-distributed -- enable distributed mode [requires -shareddir]\n"
        "-slave       -- application will run in slave mode.\n"
        );
    return output;
}

//------------------------------------------------------------------------------
/**
    Run the application.
    - If the "-jobs" parameter was given to the application
      the master mode is enabled and this class will only split the task of the
      application in jobs. These jobs are added to a job scheduler.
    - Else the overwritten DoWork() method of the subclasses is called.
*/
void
DistributedToolkitApp::Run()
{
    if(this->ParseCmdLineArgs())
    {
        if(!this->slaveArg)
        {
            // this method will execute application specific code
            this->OnBeforeRunLocal();
        }

        if(this->distributedArg)
        {
            if (this->masterMode)
            {
                // things that only master should do before execution
                // (like setting up shared directory, etc.)
                this->OnBeforeRunDistMaster();
            }
            if (this->slaveArg)
            {
                // things that every slave should do, unless there's no distribution
                // e.g. creating a lokal output directory
                this->OnBeforeRunDistSlave();
            }
        }

        // actual processing
        if (this->masterMode)
        {			
            // handle distribution and process newly created jobs
            this->CreateAndProcessJobs();
        }
        else
        {
            // perform action of specific application
            this->DoWork();
        }

        if(this->distributedArg)
        {
            if (this->slaveArg)
            {
                // things that every slave should do, unless there's no distribution
                // e.g. copying all data from lokal output directory to shared directory
                this->OnAfterRunDistSlave();
            }
            if (this->masterMode)
            {
                // things that only master should do after execution
                // (like copying all data from shared to export dir
                // and clean up of shared directory)
                this->OnAfterRunDistMaster();
            }
        }

        if(!this->slaveArg)
        {
            // this method will execute application specific code
            this->OnAfterRunLocal();
            this->WaitForKey();
        }
    }
}

//------------------------------------------------------------------------------
/**
    if waitForKey flag is set, wait until console has input, else do nothing.
*/
void
DistributedToolkitApp::WaitForKey()
{
    if (this->waitForKey)
    {
        n_printf("Press <Enter> to continue!\n");
        while (!Console::Instance()->HasInput())
        {
            Timing::Sleep(0.01);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Analyze input parameters and create a list of files.
    Then split them into jobs. Add the jobs to a 
    job scheduler and wait until the scheduler has processed all jobs.
*/
void
DistributedToolkitApp::CreateAndProcessJobs()
{
    n_assert(this->projectdirArg.IsValid());
    
    // Create a string list of all files
    Array<String> fileList = this->CreateFileList();
	    
#if 0
    n_printf("Get current revision...\n");
    // Get current svn revision of project dir
    String revision = this->GetCurrentRevision();
    if(!revision.IsValid())
    {
        n_printf("FAILED\n");
        this->SetReturnCode(10);
        return;
    }
    n_printf("OK - Revision is %s\n",revision.AsCharPtr());
#endif
    // Create jobs and split the fileList, that every job
    // gets nearly the same amount of files
    Array<Ptr<DistributedAppJob>> jobs;
    Array<Array<String>> fileLists;
    IndexT jobIndex;
    SizeT jobCount = this->jobsArg;

    // limit the files per job to maxJobCount, by adding more jobs
    const int maxJobCount = 64;
    if(fileList.Size() / jobCount > maxJobCount)
    {
        Console::Instance()->Print("Max file limit (%i) per job reached\nCreating additional jobs...",maxJobCount);
        jobCount = (SizeT)ceilf(((float)fileList.Size())/maxJobCount);
        Console::Instance()->Print("Using %i jobs now.\n\n",jobCount);
    }

    for (jobIndex = 0; jobIndex < jobCount; jobIndex++)
    {
        Ptr<DistributedAppJob> job = DistributedAppJob::Create();
        
        String exeName = this->args.GetCmdName().ExtractFileName();
#if __WIN32__
        // handle the case, that application was
        // called without .exe extension
        if(exeName.GetFileExtension().Length() == 0)
        {
            exeName.Append(".exe");
        }
#endif
        
        String appPath;
        appPath.Append("bin:" + exeName);
        
        URI appUri(appPath);
        // is the executable does not exist at the given path,
        // it maybe was'nt inside a bin/win32 directory.
        // Try the standard home: assign in that case...
        if (!IoServer::Instance()->FileExists(appUri))
        {
            appUri.Set("home:");
            appUri.AppendLocalPath(exeName);
            if (!IoServer::Instance()->FileExists(appUri))
            {
                n_printf("Application path for jobs is not valid: %s\n",appUri.AsString().AsCharPtr());
                this->SetReturnCode(10);
                return;
            }
        }
        
        // For 64 bit machine compatibility check if app path is inside
        // the programs: assign. Send the assign instead of the full path in that case.
        String localAppPath = appUri.AsString();
        String programPath =  AssignRegistry::Instance()->ResolveAssignsInString("programs:");
        localAppPath.ToLower();
        programPath.ToLower();
        IndexT idx = localAppPath.FindStringIndex(programPath);
        if (0 == idx)
        {
            String subPath = localAppPath.ExtractToEnd(programPath.Length());
            subPath.TrimLeft("/\\");
            String remoteAppPath;
            remoteAppPath.Format("programs:%s",subPath.AsCharPtr());
            job->SetApplicationPath(remoteAppPath);
        }
        else
        {
            job->SetApplicationPath(localAppPath);
        }
        
        // Setup commandline arguments for the app job
        String argString;
        argString.Append("-projectdir ");
        argString.Append(this->projectdirArg);
        // In distributed mode, the started application have
        // to know where to copy the results on a slave machine.
        if (this->distributedArg)
        {
            argString.Append(" -distributed ");
            String guidArg;
            guidArg.Format(" %s \"%s\"", "-sharedguid",
                this->sharedDirControl->GetGuid().AsString().AsCharPtr());
            argString.Append(guidArg);
            argString.Append(" -shareddir ");
            argString.Append(this->sharedDirControl->GetPath().AsString());
        }
        // The filelists are absolute. Force the export in any case.
		argString.Append(this->forceArg ? " -force" : "");
        // Append additional arguments, that are unrelated to distributed tools
        argString.Append(this->GetAdditionalArguments());
        
        job->SetArguments(argString);
        job->SetSVNPath(this->projectInfo.GetPathAttr("SVNToolPath"));
        job->SetProjectDirectory(this->projectdirArg);
       // job->SetRequiredRevision(revision);
        job->SetIdentifier(String::FromInt(jobIndex));
        job->SetDistributedFlag(this->distributedArg);
        if (this->distributedArg)
        {
            job->SetSharedDirGuid(this->sharedDirControl->GetGuid());
            job->SetSharedDirPath(this->sharedDirControl->GetPath().AsString());
        }
        jobs.Append(job);

        // prepare a filelist for the job. It is filled later.
        Array<String> list;
        fileLists.Append(list);
    }
    
    // Append files from list to jobs.
    IndexT fileIndex;
    for (fileIndex = 0; fileIndex < fileList.Size(); fileIndex++)
    {
        fileLists[fileIndex%fileLists.Size()].Append(fileList[fileIndex]);
    }

    // append jobs to scheduler
    DistributedJobScheduler scheduler;
    if (scheduler.Open())
    {  
        // setup scheduler for distributed build
        scheduler.UseRemoteServices(this->distributedArg);
        scheduler.SetMaxParallelLocalJobs(this->processorsArg);
        if (this->projectInfo.HasAttr("BuildSlaves"))
        {
            String attr = this->projectInfo.GetAttr("BuildSlaves");
            Array<String> addresses = attr.Tokenize(";");
            IndexT a;
            for(a = 0; a < addresses.Size(); a++)
            {
                Array<String> addressParts = addresses[a].Tokenize(":");
                if(addressParts.Size()==2)
                {
                    Ptr<RemoteJobService> service = RemoteJobService::Create();
                    Net::IpAddress ipAddress;
                    ipAddress.SetHostName(addressParts[0]);
                    ipAddress.SetPort(addressParts[1].AsInt());
                    service->SetIpAddress(ipAddress);
                    service->SetVerboseFlag(this->verboseFlag);
                    scheduler.AttachRemoteJobService(service);
                }
            }
        }
        // attach a file list to each job and add the job to the scheduler
        for (jobIndex = 0; jobIndex < jobCount; jobIndex++)
        {
            jobs[jobIndex]->SetFileList(fileLists[jobIndex]);
            scheduler.AttachJob(jobs[jobIndex].upcast<DistributedJob>());
        }
        
        // attach initialize/finalize jobs to scheduler
        Array<Ptr<DistributedJob>> initJobs = this->GenerateInitializeJobList();
        IndexT j;
        for (j = 0; j < initJobs.Size(); j++)
        {
        	scheduler.AppendInitializeJob(initJobs[j]);
        }
        Array<Ptr<DistributedJob>> finJobs = this->GenerateFinalizeJobList();
        for (j = 0; j < finJobs.Size(); j++)
        {
            scheduler.AppendFinalizeJob(finJobs[j]);
        }
        
        // run all jobs attached to the scheduler
        scheduler.RunJobs();
        // wait until the scheduler has finished...
        while (scheduler.HasActiveJobs())
        {
            scheduler.Update();
            Timing::Sleep(1);
        }
        scheduler.Close();
    }
}

//------------------------------------------------------------------------------
/**
    Checks input arguments and build a string list of all files
    which should processed by the application. Override this in
    subclasses if you need another way of creation for this filelist.
*/
Array<String>
DistributedToolkitApp::CreateFileList()
{
    Array<String> arr;
    
    if (this->listfileArg.IsValid())
    {
        // read file lines and add them to the list
        URI listUri;
        listUri.Set(this->listfileArg);
        
        Ptr<Stream> readStream = IoServer::Instance()->CreateStream(listUri);
        readStream->SetAccessMode(Stream::ReadAccess);
        readStream->Open();
        
        Ptr<TextReader> reader = TextReader::Create();
        reader->SetStream(readStream);
        reader->Open();
        while(!reader->Eof())
        {
            String line = reader->ReadLine();
            line.Trim(" \r\n");
            arr.Append(line);
        }
        reader->Close();
        reader = nullptr;

        readStream->Close();
        readStream = nullptr;
    }
    else if (this->dirArg.IsValid())
    {
        if(this->fileArg.IsValid())
        {
            // just add this file to the list
            String str;
            str.Append(this->projectdirArg);
            str.Append("/");
            str.Append(this->dirArg);
            str.Append("/");
            str.Append(this->fileArg);
            arr.Append(str);
        }
        else
        {
            // add all files in the specified directory to the list
            URI dirUri;
            dirUri.Set(this->projectdirArg);
            dirUri.AppendLocalPath(this->dirArg);
            
            Array<String> fileList = IoServer::Instance()->ListFiles(dirUri,"*.*");
            
            IndexT i;
            for (i=0;i<fileList.Size();i++)
            {
                String str;
                str.Append(this->projectdirArg);
                str.Append("/");
                str.Append(this->dirArg);
                str.Append("/");
                str.Append(fileList[i].ExtractFileName());
                arr.Append(str);
            }
        }
    }
    return arr;
}
//------------------------------------------------------------------------------
/**
	gets svn info of the current project dir and returns revision id
*/
String
DistributedToolkitApp::GetCurrentRevision()
{
    String out;
    
    int tries = 0;
    do
    {
        out.Clear();

        ToolkitUtil::AppLauncher svnApp;

        String svnArgs;
        svnArgs.Append("info");
        svnApp.SetArguments(svnArgs);

        URI svnUri;
        svnUri.Set(this->projectInfo.GetPathAttr("SVNToolPath"));
        svnApp.SetExecutable(svnUri);

        URI svnWorkUri;    
        svnWorkUri.Set(this->projectdirArg);
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

        if(!out.IsValid())
        {
            Console::Instance()->Print("Failed to get current svn revision. Retry in 15 seconds...\n");
            n_sleep(15);
            tries++;
        }
    } while (!out.IsValid() && tries < 3);
    if (!out.IsValid())
    {
        Console::Instance()->Error("Failed to get svn revision.\n");
    }
    return out;
}

//------------------------------------------------------------------------------
/**
    Do the work of the application (override in subclasses!)	
*/
void
DistributedToolkitApp::DoWork()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Subclasses may define their own command line arguments.
    This method analyzes all command line arguments and
    return the ones, which are not needed by this class.
    The filtered string will shared to slave applications for sure.
*/
String
DistributedToolkitApp::GetAdditionalArguments()
{
    String output;
    SizeT argCount = this->args.GetNumArgs();
    IndexT argIndex = 0;
    while (argIndex < argCount)
    {
        String arg = this->args.GetStringAtIndex(argIndex);
        if(
            arg == "-jobs" ||
            arg == "-projectdir" ||
            arg == "-dir" ||
            arg == "-file" ||
            arg == "-listfile" ||
            arg == "-shareddir" ||
            arg == "-processors"
            )
        {
            // ignore next arg because its a value
            argIndex++;
        }
        else if (
            arg != "-help" &&
            arg != "-waitforkey" &&
            arg != "-slave" &&
            arg != "-force"
            )
        {
            // this arg must be an additional argument
            output.Append(" ");
            output.Append(arg);
        }
        argIndex++;
    }
    return output;
}




//------------------------------------------------------------------------------
/**
    things that every slave should do before execution,
    unless it is not distributed
*/
void
DistributedToolkitApp::OnBeforeRunDistSlave()
{
    Console::Instance()->Print("Create/Cleanup export directory\n");
    // Create destination directory, if it does not exist,
    // else clean it up, by deleting all content in it.
    URI dst("dst:");
    if (!IoServer::Instance()->DirectoryExists(dst))
    {
        if (!IoServer::Instance()->CreateDirectory(dst))
        {
            Console::Instance()->Error("Could not create destination directory: %s\n", dst.AsString().AsCharPtr());
        }
    }
    else
    {
        this->DeleteDirectoryContent(dst);
    }
}

//------------------------------------------------------------------------------
/**
    things that every slave should do after execution,
    unless it is not distributed
*/
void
DistributedToolkitApp::OnAfterRunDistSlave()
{
    // copy data from destination dir to shared dir
    Console::Instance()->Print("Copy files to shared directory...\n");
    URI dst("dst:");
    this->sharedDirControl->CopyDirectoryContentToSharedDir(dst.LocalPath());
    // delete destination dir
    this->DeleteDirectoryContent(dst);
    Console::Instance()->Print("Done\n");
}

//------------------------------------------------------------------------------
/**
    things that only master should do before execution
    unless it is not distributed
*/
void
DistributedToolkitApp::OnBeforeRunDistMaster()
{
    // master creates a subdirectory inside of shared dir from guid.
    // after a job has finished, it's data will be written from the
    // destination directory to this subdirectory
    this->sharedDirControl->SetUp();
}

//------------------------------------------------------------------------------
/**
    things that only master should do after execution
    unless it is not distributed
*/
void
DistributedToolkitApp::OnAfterRunDistMaster()
{
    // copy everything out of shared dir into lokal export
    Console::Instance()->Print("Copy files from shared directory to export\n");
    this->sharedDirControl->CopyDirectoryContentFromSharedDir(URI("dst:").LocalPath());
    // finally remove shared dir content (like guid subfolder, etc.)
    this->sharedDirControl->CleanUp();
    Console::Instance()->Print("Done\n");
}

//------------------------------------------------------------------------------
/**
    Actions that are done at local machine before any work (override in subclasses)	
*/
void
DistributedToolkitApp::OnBeforeRunLocal()
{
}
//------------------------------------------------------------------------------
/**
    Actions that are done at local machine after any work (override in subclasses)	
*/
void
DistributedToolkitApp::OnAfterRunLocal()
{
}
//------------------------------------------------------------------------------
/**
    Generate a list of jobs that are added to the scheduler as jobs which are
    processed before a group of parallel jobs is started on a host
*/
Array<Ptr<DistributedJob>>
DistributedToolkitApp::GenerateInitializeJobList()
{
    return Array<Ptr<DistributedJob>>();
}
//------------------------------------------------------------------------------
/**
	Generate a list of jobs that are added to the scheduler as jobs which are
    processed after a group of parallel jobs has finished
*/
Array<Ptr<DistributedJob>>
DistributedToolkitApp::GenerateFinalizeJobList()
{
    return Array<Ptr<DistributedJob>>();
}

//------------------------------------------------------------------------------
/**
	removes all content of given directory recursively 
*/
void
DistributedToolkitApp::DeleteDirectoryContent(const IO::URI & path)
{
    Array<String> files = IoServer::Instance()->ListFiles(path, "*");
    IndexT fileIndex;
    for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
    {
        URI file(path);
        file.AppendLocalPath(files[fileIndex]);
        IoServer::Instance()->DeleteFile(file);
    }
    Array<String> dirs = IoServer::Instance()->ListDirectories(path, "*");
    IndexT dirIndex;
    for (dirIndex = 0; dirIndex < dirs.Size(); dirIndex++)
    {
        URI dir(path);
        dir.AppendLocalPath(dirs[dirIndex]);
        this->DeleteDirectoryContent(dir);
        IoServer::Instance()->DeleteDirectory(dir);
    }
}

} // namespace DistributedTools
