//------------------------------------------------------------------------------
//  texturebatcherapp.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "texturebatcherapp.h"
#include "toolkitutil/platform.h"
#include "io/ioserver.h"
#include "io/textreader.h"
#include "io/console.h"
#include "io/uri.h"
#include "timing/time.h"

namespace Toolkit
{
using namespace ToolkitUtil;
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
bool
TextureBatcherApp::ParseCmdLineArgs()
{   
    // parse distributed args
    if (DistributedToolkitApp::ParseCmdLineArgs())
    {
        // always use outputDir as destination directory, even if
        // application is not distributed. In that case outputDir
        // is set to this->exportdirArg anyway.
        this->textureConverter.SetDstDir(this->projectInfo.GetPathAttr("TextureDstDir"));
        this->textureConverter.SetForceFlag(this->forceArg);
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------
/**
    derived from DistributedToolkitApp. Also checks for timestamps and force flags.
*/
Array<String>
TextureBatcherApp::CreateFileList()
{
    Array<String> res;

    // create list from given filelist
    if (this->listfileArg.IsValid())
    {
        URI listUri(this->listfileArg);
        // open stream and reader
        Ptr<Stream> readStream = IoServer::Instance()->CreateStream(listUri);
        readStream->SetAccessMode(Stream::ReadAccess);
        Ptr<TextReader> reader = TextReader::Create();
        reader->SetStream(readStream);
        if (reader->Open())
        {
            // read each line and append to list
            while(!reader->Eof())
            {
                String srcPath = reader->ReadLine();
                srcPath.Trim(" \r\n");
                if (this->NeedsConversion(srcPath) || this->forceArg)
                {
                    res.Append(srcPath);
                }
            }
            // close stream and reader
            reader->Close();
        }
        reader = 0;
        readStream = 0;
    }
    // read directory content
    else if (this->dirArg.IsValid())
    {
        // just add a single file to list
        if (this->fileArg.IsValid())
        {
            String src;
            src.Format("src:assets/%s/%s",
                this->dirArg.AsCharPtr(),
                this->fileArg.AsCharPtr()
                );
            if (this->NeedsConversion(src) || this->forceArg)
            {
                res.Append(src);
            }
        }
        // add all files in this dir
        else
        {
            String directory;
            directory.Format("src:assets/%s",
                this->dirArg.AsCharPtr()
                );
            res.AppendArray(this->GetFileListFromDirectory(directory));
        }
    }
    // if there are no arguments whatsoever add all files in texture dir to list
    else
    {
        Array<String> dirs = IoServer::Instance()->ListDirectories("src:assets", "*");
        IndexT dirIndex;
        // iterate through each subdirectory of "textures"
        for (dirIndex = 0; dirIndex < dirs.Size(); dirIndex++)
        {
            if (dirs[dirIndex] != ".svn")
            {
                String directory;
                directory.Format("src:assets/%s", dirs[dirIndex].AsCharPtr());
                res.AppendArray(this->GetFileListFromDirectory(directory));
            }
        }
    }
    return res;
}

//------------------------------------------------------------------------------
/**
*/
bool
TextureBatcherApp::SetupProjectInfo()
{
    if (DistributedToolkitApp::SetupProjectInfo())
    {
        this->textureConverter.SetPlatform(this->platform);
        if (this->projectInfo.HasAttr("TextureTool"))
        {
            this->textureConverter.SetToolPath(this->projectInfo.GetPathAttr("TextureTool"));
        }
        this->textureConverter.SetTexAttrTablePath(this->projectInfo.GetAttr("TextureAttrTable"));
        this->textureConverter.SetDstDir(this->projectInfo.GetAttr("TextureDstDir"));
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureBatcherApp::ShowHelp()
{
    n_printf("NebulaT texture batch exporter.\n"
             "(C) Individual Authors, see AUTHORS file 2013.\n");
    n_printf(this->GetArgumentDescriptionString().AsCharPtr());
    n_printf("-force       -- force export (don't check time stamps)\n"
             "-platform    -- select platform (win32, linux)\n\n");
}

//------------------------------------------------------------------------------
/**
*/
void
TextureBatcherApp::DoWork()
{    
    // get a list of files to convert
    Array<String> files = this->CreateFileList();
    if (files.Size() > 0)
    {
        // setup the texture converter
        Console::Instance()->Print("Set up Texture Converter\n");
        if (!this->textureConverter.Setup(this->logger))
        {
            n_printf("ERROR: failed to setup texture converter!\n");
            this->SetReturnCode(-1);
            return;
        }
        // perform texture conversion
        Console::Instance()->Print("Start Texture Conversion\n");
        bool result = this->textureConverter.ConvertFiles(files);
        Console::Instance()->Print("Done\n");
        this->textureConverter.Discard();
    }
}

//------------------------------------------------------------------------------
/**
    Return all files inside this directory that need a texture conversion.
*/
Array<String>
TextureBatcherApp::GetFileListFromDirectory(const String& directory)
{
    Array<String> res;
    Array<String> files = IoServer::Instance()->ListFiles(directory, "*");
    IndexT fileIndex;
    // check each file in this directory if it needs to be converted
    for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
    {
        String srcPath;
        srcPath.Format("%s/%s", directory.AsCharPtr(), files[fileIndex].AsCharPtr());
        if (this->NeedsConversion(srcPath))
        {
            res.Append(srcPath);
        }
    }
    return res;
}

//------------------------------------------------------------------------------
/**
    Return platform dependent file extension for converted textures.
*/
String
TextureBatcherApp::GetDstTextureFileExtension()
{
    if(Platform::Win32 == this->platform)
    {
        return "dds";
    }    
    else if (Platform::Linux == this->platform)
    {
        return "dds";
    }
    else
    {
        n_error("TextureBatcherApp::GetDstTextureFileExtension(): invalid platform!\n");
        return "<INVALID>";
    }
}

//------------------------------------------------------------------------------
/**
    Perform a file time check to decide whether a texture must be
    converted. Build dstPath from project settings and file name of srcPath.
*/
bool
TextureBatcherApp::NeedsConversion(const String& srcPath)
{
    // cut source dir part from srcPath
    String resolvedSrc = AssignRegistry::Instance()->ResolveAssignsInString(srcPath);
    String subPath = resolvedSrc.ExtractToEnd(AssignRegistry::Instance()->ResolveAssignsInString("src:assets").Length());
    subPath.Trim("/");
    String dstPath;
    dstPath.Format("%s/%s",
        this->projectInfo.GetPathAttr("TextureDstDir").AsCharPtr(),
        subPath.AsCharPtr()
    );
    // be sure to remove all file extensions
    while ("" != dstPath.GetFileExtension())
    {
        dstPath.StripFileExtension();
    }
    dstPath.Format("%s.%s", dstPath.AsCharPtr(), this->GetDstTextureFileExtension().AsCharPtr());
    return this->NeedsConversion(srcPath, dstPath);
}

//------------------------------------------------------------------------------
/**
    Perform a file time check to decide whether a texture must be
    converted. Use an explicit dstPath
*/
bool
TextureBatcherApp::NeedsConversion(const String& srcPath, const String& dstPath)
{
    // file time check overriden?
    if (this->forceArg)
    {
        return true;
    }
    // otherwise check file times of src and dst file
    IoServer* ioServer = IoServer::Instance();
    if (ioServer->FileExists(dstPath))
    {
        FileTime srcFileTime = ioServer->GetFileWriteTime(srcPath);
        FileTime dstFileTime = ioServer->GetFileWriteTime(dstPath);
        FileTime attrFileTime = srcFileTime;
        String xml = srcPath;
        xml.StripFileExtension();
        xml += ".xml";
        if(ioServer->FileExists(xml))
        {
            attrFileTime = ioServer->GetFileWriteTime(xml);
        }
		if(attrFileTime > dstFileTime)
		{
			return true;
		}
        if (dstFileTime > srcFileTime)
        {
            // dst file newer then src file, don't need to convert
            return false;
        }
    }
    // fallthrough: dst file doesn't exist, or it is older than the src file
    return true;
}
} // namespace ToolkitUtil
