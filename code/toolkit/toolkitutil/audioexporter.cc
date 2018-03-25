//------------------------------------------------------------------------------
//  audioexporter.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "audioexporter.h"
#include "io/ioserver.h"
#include "io/uri.h"
#include "toolkitutil/applauncher.h"
#include "io/memorystream.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
AudioExporter::AudioExporter() :
    platform(Platform::Win32)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AudioExporter::~AudioExporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
AudioExporter::Export()
{
    if ((this->platform == Platform::Win32) || (this->platform == Platform::Linux))
    {
        return this->ExportFmod();
    }    
    else
    {
        n_printf("WARNING: unsupported platform '%s' in AudioExporter!\n", Platform::ToString(this->platform).AsCharPtr());
        return true;
    }
}

//------------------------------------------------------------------------------
/**
    Audio export function for Windows and Xbox360 platform.
*/
bool
AudioExporter::ExportXact()
{
    n_assert(this->toolPath.IsValid());
    n_assert(this->projectFile.IsValid());      
    n_assert(this->dstDir.IsValid());
    IoServer* ioServer = IoServer::Instance();

    // resolve assigns in project file path
    String resolvedProjFilePath = AssignRegistry::Instance()->ResolveAssignsInString(this->projectFile);
    if (!ioServer->FileExists(resolvedProjFilePath))
    {
        n_printf("WARNING: XACT project file '%s' does not exist!\n", resolvedProjFilePath.AsCharPtr());
        return false;
    }
    
    // resolve assigns in destination dir and make sure that path exists
    String resolvedDstPath = AssignRegistry::Instance()->ResolveAssignsInString(this->dstDir);
    ioServer->CreateDirectory(this->dstDir);

    // setup command line args and launch xactbld3.exe
    String args = "/L /X:HEADER /X:CUELIST /X:REPORT ";
    if (this->force)
    {
        args.Append("/F ");
    }
    if (Platform::Win32 == this->platform)
    {
        args.Append("/WIN32 ");
    }
    else
    {
        args.Append("/XBOX360 ");
    }
    args.Append("\"");
    args.Append(resolvedProjFilePath);
    args.Append("\" \"");
    args.Append(resolvedDstPath);
    args.Append("\"");

    AppLauncher appLauncher;
    appLauncher.SetExecutable(this->toolPath);
    appLauncher.SetWorkingDirectory(resolvedDstPath);
    appLauncher.SetArguments(args);
    if (!appLauncher.LaunchWait())
    {
        n_printf("WARNING: failed to launch audio tool '%s'!\n", this->toolPath.AsCharPtr());
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Audio export function for Wii platform.
*/
bool
AudioExporter::ExportWii()
{
	n_assert(this->toolPath.IsValid());
	n_assert(this->srcDir.IsValid());
    n_assert(this->dstDir.IsValid());

	IoServer* ioServer = IoServer::Instance();

	// find all SoundMaker projectfiles in the audio folder
	this->FindRspjFiles(this->srcDir);
	
	// convert all SoundMaker projectfiles (".rspj") to SoundArchives (".brsar")
	AppLauncher convAppLauncher;
    convAppLauncher.SetExecutable(this->toolPath);
	convAppLauncher.SetWorkingDirectory("proj:work");
	IndexT i;
    for (i = 0; i < this->rspjFiles.Size(); i++)
    {
		convAppLauncher.SetArguments(AssignRegistry::Instance()->ResolveAssignsInString(this->rspjFiles[i]));
		if (!convAppLauncher.LaunchWait())
		{
			n_error("Failed to launch '%s %s'!\n", this->toolPath.AsCharPtr(), this->rspjFiles[i].AsCharPtr());
			return false;
		}
    }

	// copy converted files to destination
	String localDstPath = AssignRegistry::Instance()->ResolveAssignsInString(this->dstDir);
	for (i = 0; i < this->rspjFiles.Size(); i++)
    {
		// create destination folder
		String destFolder = this->rspjFiles[i];
		destFolder.SubstituteString("src:audio", "");
		destFolder.TerminateAtIndex(destFolder.FindStringIndex("/rspj"));
		destFolder = localDstPath + destFolder;
		if (!ioServer->DirectoryExists(destFolder))
		{
			ioServer->CreateDirectory(destFolder);
		}

		// copy files from source folder to destination folder
		String srcFolder = AssignRegistry::Instance()->ResolveAssignsInString(this->srcFolders[i]);
		this->CopyDirectory(srcFolder, destFolder);
    }
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioExporter::FindRspjFiles(String folder)
{
	IoServer* ioServer = IoServer::Instance();

	// look for rspj files
	Array<String> rspjFiles = ioServer->ListFiles(folder, "*.rspj");
	IndexT i;
    for (i = 0; i < rspjFiles.Size(); i++)
    {
		this->rspjFiles.Append((folder + "/" + rspjFiles[i]));
		this->srcFolders.Append(folder + "/" + "output/dvddata");
    }

	// step through all subfolders
	Array<String> subfolders = ioServer->ListDirectories(folder, "*");
	for (i = 0; i < subfolders.Size(); i++)
    {
		if ((subfolders[i] != "CVS") && (subfolders[i] != ".svn"))
        {
			this->FindRspjFiles((folder + "/" + subfolders[i]));
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AudioExporter::CopyDirectory(Util::String src, Util::String dst)
{	
	IoServer* ioServer = IoServer::Instance();

	// copy all files in the directory
	Array<String> files = ioServer->ListFiles(src, "*");
	IndexT i;
    for (i = 0; i < files.Size(); i++)
    {
		ioServer->CopyFile(src + "/" + files[i], dst + "/" + files[i]);
    }

	// copy all subdirectories
	Array<String> directories = ioServer->ListDirectories(src, "*");
	for (i = 0; i < directories.Size(); i++)
    {
		ioServer->CreateDirectory(dst + "/" + directories[i]);
		this->CopyDirectory(src + "/" + directories[i], dst + "/" + directories[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AudioExporter::FmodFindProjectFiles(const Util::String &folder, Util::Array<Util::String> &projectFiles, const Util::String & pattern) const
{
	IoServer* ioServer = IoServer::Instance();
	projectFiles.Clear();
	Array<String> directories = ioServer->ListDirectories(folder, "*");
	for (IndexT j = 0; j < directories.Size(); j++)
	{
		// look for fmod project files in folder
		Util::Array<Util::String> projs = ioServer->ListFiles(folder + "/" + directories[j], pattern);
		IndexT i;
		for (i = 0; i < projs.Size(); i++)
		{
			projectFiles.Append(folder + "/" + directories[j] + "/" + projs[i]);
		}
	}	
}


//------------------------------------------------------------------------------
/**
Audio export function for fmod studio.
*/
bool
AudioExporter::ExportFmod()
{
	n_assert(this->toolPath.IsValid());
	n_assert(this->srcDir.IsValid());
	n_assert(this->dstDir.IsValid());
	IoServer* ioServer = IoServer::Instance();


	// 1. create audio export directory
	// resolve assigns in destination dir and make sure that path exists
	const String resolvedDstPath = AssignRegistry::Instance()->ResolveAssignsInString(this->dstDir);
	const String resolvedSrcPath = AssignRegistry::Instance()->ResolveAssignsInString(this->srcDir);
	ioServer->CreateDirectory(this->dstDir);

	// 2. collect all *.fspro files (fmod studio project file)
	Util::Array<Util::String> projects;
	this->FmodFindProjectFiles(resolvedSrcPath, projects, "*.fspro");
	if (projects.Size() == 0)
	{
		n_printf("no audio projects found\n");
		return false;
	}
	n_assert2(projects.Size() == 1, "Multiple fsproj files found, only one is supported at this time");

	Util::String baseArgs = "-build ";
	switch (this->platform)
	{
	case Platform::Win32:
	case Platform::Linux:		
		break;
	default:
		n_error("platform not supported\n");
		break;
	}
	
	
	String projectName;
				
	String args = baseArgs + "\"" + projects[0] + "\"";

	// since fmod studio has no useful way of using their commandline tool we have to do some magic here
	Array<String> paths = ioServer->ListDirectories(this->toolPath, "FMOD Studio 1*");
	n_assert2(paths.Size() > 0, "No FMOD Studio application directory found");

	String executable = this->toolPath + "/" + paths[paths.Size() - 1] + "/fmodstudiocl.exe";
	AppLauncher appLauncher;
	appLauncher.SetExecutable(executable);
	appLauncher.SetWorkingDirectory(resolvedDstPath);
	appLauncher.SetArguments(args);
	Ptr<IO::MemoryStream> ss = IO::MemoryStream::Create();
	appLauncher.SetStderrCaptureStream(ss.cast<IO::Stream>());

	if (!appLauncher.LaunchWait())
	{
		n_printf("WARNING: failed to launch audio tool '%s'!\n", executable.AsCharPtr());
		return false;
	}
	
	// copy build files to output folder
	Array<String> files = ioServer->ListFiles(projects[0].ExtractDirName() + "/Build/Desktop/","*.bank");

	for (int i = 0; i < files.Size(); i++)
	{
		ioServer->CopyFile(projects[0].ExtractDirName() + "/Build/Desktop/" + files[i], resolvedDstPath + "/" + files[i]);
	}

	//////////////////////////////////////
	// FIXME: delete files we done need anymore (.cache Build)	
	return true;
}

} // namespace ToolkitUtil
