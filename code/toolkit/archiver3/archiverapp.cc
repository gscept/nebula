//------------------------------------------------------------------------------
//  archiveapp.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "archiverapp.h"
#include "toolkitutil/applauncher.h"
#include "io/xmlwriter.h"
#include "zlib/zlib.h"
#include "io/binarywriter.h"

namespace Toolkit
{
using namespace ToolkitUtil;
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
ArchiverApp::ArchiverApp() :
    webDeployFlag(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ArchiverApp::ShowHelp()
{
    n_printf("NebulaT archiver.\n"
             "(C) Radon Labs GmbH\n"
             "Creates platform-specific asset archives (e.g. export.zip, export_win32.zip)\n"
             "-help -- display this help\n"
             "-webdeploy -- create a web-deployment directory (only win32 platform)!\n");
}

//------------------------------------------------------------------------------
/**
*/
bool
ArchiverApp::ParseCmdLineArgs()
{
    if (ToolkitApp::ParseCmdLineArgs())
    {
        this->webDeployFlag = this->args.GetBoolFlag("-webdeploy");
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
ArchiverApp::SetupProjectInfo()
{
    if (ToolkitApp::SetupProjectInfo())
    {
        if (this->projectInfo.HasAttr("ArchiverTool"))
        {
            this->toolPath = this->projectInfo.GetPathAttr("ArchiverTool");
        }
        else
        {
            n_printf("ERROR: no ArchiveTool attribute set in projectinfo.xml!");
            return false;
        }
        if (this->projectInfo.HasAttr("ArchiverExcludePatterns"))
        {
            this->excludePatterns = this->projectInfo.GetAttr("ArchiverExcludePatterns").Tokenize("; ");
        }
        else
        {
            this->excludePatterns.Append("*.db4");
        }        
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
ArchiverApp::Run()
{
    // parse command line args
    if (!this->ParseCmdLineArgs())
    {
        return;
    }

    // setup the project info object
    if (!this->SetupProjectInfo())
    {
        return;
    }
    
    // invoke platformspecific packers
    switch (this->platform)
    {
        case Platform::Win32:
            if (this->webDeployFlag)
            {
                this->PackWebDeploy(this->projectInfo.GetAttr("DstDir"), "webdeploy_data");
                break;
            }
            // fallthrough!
        case Platform::Linux:
            this->PackDirectoryWin360(this->projectInfo.GetAttr("DstDir"));
            break;
    }
}

//------------------------------------------------------------------------------
/**
    Creates web-deployment data. This recursively every file and copies
    it into the webdeployment directory.
*/
void
ArchiverApp::PackWebDeploy(const String& srcDir, const String& webDeployDir)
{
    // recursively copy directories
    String srcPath = srcDir;
    String dstPath = "proj:" + webDeployDir + "/" + srcDir.ExtractFileName();
    this->RecursePackWebDeployDirectory(srcPath, dstPath);
}

//------------------------------------------------------------------------------
/**
    Recursively handle a single directory for web-deployment.
*/
void
ArchiverApp::RecursePackWebDeployDirectory(const String& srcDir, const String& dstDir)
{
    IoServer* ioServer = IoServer::Instance();
    
    // first make sure the destination directory exists
    ioServer->CreateDirectory(dstDir);

    // compress-copy all files in directory
    Array<String> files = ioServer->ListFiles(srcDir, "*");
    IndexT fileIndex;
    for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
    {
        String srcPath = srcDir + "/" + files[fileIndex];
        String dstPath = dstDir + "/" + files[fileIndex] + ".nz";
        this->CompressCopyFile(srcPath, dstPath);
    }

    // recurse into subdirectories
    Array<String> dirs = ioServer->ListDirectories(srcDir, "*");
    IndexT dirIndex;
    for (dirIndex = 0; dirIndex < dirs.Size(); dirIndex++)
    {
        const String& curDir = dirs[dirIndex];
        if ((curDir != "CVS") && (curDir != ".svn"))
        {
            String srcPath = srcDir + "/" + curDir;
            String dstPath = dstDir + "/" + curDir;
            this->RecursePackWebDeployDirectory(srcPath, dstPath);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Compress and copy a single file for web deployment.
*/
void
ArchiverApp::CompressCopyFile(const String& srcPath, const String& dstPath)
{
    IoServer* ioServer = IoServer::Instance();

    // open source file
    Ptr<Stream> srcStream = ioServer->CreateStream(srcPath);
    if (srcStream->Open())
    {
        if (srcStream->GetSize() == 0)
        {
            n_printf("* skipping '%s' (file is empty)\n", srcPath.AsCharPtr());
            return;
        }
        void* srcData = srcStream->Map();
        SizeT srcSize = srcStream->GetSize();

        // get max compressed len from zlib and allocate compression buffer
        uLong maxSize = compressBound(srcSize);
        void* buf = Memory::Alloc(Memory::ScratchHeap, maxSize);

        // compress the source data into the compression buffer
        uLongf ioDestLen = maxSize;
        int res = compress2((Bytef*)buf, &ioDestLen, (const Bytef*)srcData, srcSize, 9);
        n_assert(Z_OK == res);

        // write destination file with compressed data
        Ptr<Stream> dstStream = ioServer->CreateStream(dstPath);
        Ptr<BinaryWriter> writer = BinaryWriter::Create();
        writer->SetStream(dstStream);
        if (writer->Open())
        {
            writer->WriteUInt(FourCC('NZ__').AsUInt());
            writer->WriteUInt(srcSize);
            writer->WriteRawData(buf, ioDestLen);
            writer->Close();

            n_printf("-> %s (%d -> %d)\n", srcPath.AsCharPtr(), srcSize, ioDestLen);
        }
        else
        {
            n_error("ArchiverApp::CompressCopyFile(): failed to open dst file '%s'!\n", dstPath.AsCharPtr());
        }

        // cleanup everything
        Memory::Free(Memory::ScratchHeap, buf);
        buf = 0;
        srcStream->Unmap();
        srcStream->Close();
        srcStream =  0;
    }
    else
    {
        n_error("ArchiverApp::CompressCopyFile: failed to open src file '%s'!\n", srcPath.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    Packs a single directory using zip.exe.
*/
void
ArchiverApp::PackDirectoryWin360(const String& dirPath)
{
    IoServer* ioServer = IoServer::Instance();

    // make sure the directory exists
    if (!ioServer->DirectoryExists(dirPath))
    {
        n_printf("ERROR: dir '%s' does not exist!", dirPath.AsCharPtr());
        return;
    }

    // delete the target file, if exists
    String filePath = dirPath + ".zip";
    if (ioServer->FileExists(filePath))
    {
        ioServer->DeleteFile(filePath);
    }

    // invoke the zip tool
    String args;
    args.Format("-r %s %s\\* ", filePath.ExtractFileName().AsCharPtr(), dirPath.ExtractFileName().AsCharPtr());    
    IndexT i;
    for (i = 0; i < this->excludePatterns.Size(); i++)
    {
        args.Append("-x ");
        args.Append(this->excludePatterns[i]);
        args.Append(" ");
    }

    n_printf("Archiving: %s %s", this->toolPath.AsCharPtr(), args.AsCharPtr());
    AppLauncher appLauncher;
    appLauncher.SetExecutable(this->toolPath);
    appLauncher.SetWorkingDirectory("proj:");
    appLauncher.SetArguments(args);
    if (!appLauncher.LaunchWait())
    {
        n_printf("WARNING: failed to launch converter tool '%s'!\n", this->toolPath.AsCharPtr());
    }
}



} // namespace Toolkit
