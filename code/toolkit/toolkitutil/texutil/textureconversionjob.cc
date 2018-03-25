//------------------------------------------------------------------------------
// textureconversionjob.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "textureconversionjob.h"
#include "io/ioserver.h"

namespace ToolkitUtil
{
using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
    Constructor	
*/
TextureConversionJob::TextureConversionJob() :
    textureAttrTable(0),
    logger(0),
    force(false),
    quiet(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Start the conversion job. Override in subclasses.	
*/
bool
TextureConversionJob::Convert()
{
    n_assert(this->srcPath.IsValid());
    n_assert(this->dstPath.IsValid());
    n_assert(this->tmpDir.IsValid());    
    n_assert(this->dstFileExt.IsValid());
    n_assert(0 != this->textureAttrTable);
    n_assert(0 != this->logger);

    // set dst file extension
    if (this->PrepareConversion(srcPath, dstPath))
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
/**
    Prepares the actual conversion process, first checks if conversion
    is necessary, removes write protection on target file, and copies
    the file if the source is already in native format. Returns true
    if no further conversion is needed.
*/
bool
TextureConversionJob::PrepareConversion(const String& srcPath, const String& dstPath)
{
    IoServer* ioServer = IoServer::Instance();

    // first make sure the target directory exists
    IoServer::Instance()->CreateDirectory(this->dstPath.ExtractDirName());

    // check if we can skip conversion based on the file time stamps and force flag
    if (!this->NeedsConversion(srcPath, dstPath))
    {
		n_printf("Up to date texture: %s\n", srcPath.AsCharPtr());
        return true;
    }

    // remove read-only attr from dst file 
    if (ioServer->FileExists(dstPath))
    {
        ioServer->SetReadOnly(dstPath, false);
    }

    // setup the path of the temporary destination file
    this->tmpPath.Append(this->tmpDir);
    this->tmpPath.Append("/");
    this->tmpPath.Append(this->dstPath.ExtractFileName());

    // make sure the temp directory exists
    IoServer::Instance()->CreateDirectory(this->tmpPath.ExtractDirName());

    // get texture conversion attributes
    String texEntry;
    texEntry.Format("%s/%s", this->srcPath.ExtractLastDirName().AsCharPtr(), this->srcPath.ExtractFileName().AsCharPtr());
	texEntry.StripFileExtension();
    this->textureAttrs = this->textureAttrTable->GetEntry(texEntry);
    
    // if destination file is already in native format, do a plain copy
    if (srcPath.GetFileExtension() == dstPath.GetFileExtension())
    {
        ioServer->CopyFile(srcPath, dstPath);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Perform a file time check to decide whether a texture must be
    converted.
*/
bool
TextureConversionJob::NeedsConversion(const String& srcPath, const String& dstPath)
{
    // file time check overriden?
    if (this->force)
    {
        return true;
    }

    // otherwise check file times of src and dst file
    IoServer* ioServer = IoServer::Instance();
    if (ioServer->FileExists(dstPath))
    {
        FileTime srcFileTime = ioServer->GetFileWriteTime(srcPath);
        FileTime dstFileTime = ioServer->GetFileWriteTime(dstPath);
        FileTime attrTime = srcFileTime;
        String texEntry;
        texEntry.Format("%s/%s", srcPath.ExtractLastDirName().AsCharPtr(), srcPath.ExtractFileName().AsCharPtr());
        texEntry.StripFileExtension();
        
		if (this->textureAttrTable->HasEntry(texEntry))
        {
            attrTime = this->textureAttrTable->GetEntry(texEntry).GetTime();
        }
        if (dstFileTime > srcFileTime && dstFileTime > attrTime)
        {
            // dst file newer then src file, don't need to convert
            return false;
        }
    }

    // fallthrough: dst file doesn't exist, or it is older then the src file
    return true;
}

//------------------------------------------------------------------------------
/**
    Copies the converted texture from the temp directory to the destination
    directory and deletes the temp file and directory.
*/
bool
TextureConversionJob::CopyResult()
{
    n_assert(0 != this->logger);
    bool retval = true;
    IoServer* ioServer = IoServer::Instance();
    if (ioServer->FileExists(this->tmpPath))
    {
        ioServer->CopyFile(this->tmpPath, this->dstPath);
    }
    else
    {
        this->logger->Error("Texture copy failed because src file does not exist: %s -> %s \n", 
            this->tmpPath.AsCharPtr(), this->dstPath.AsCharPtr());
        retval = false;
    }
    ioServer->DeleteFile(this->tmpPath);
    ioServer->DeleteDirectory(this->tmpDir);
    return retval;
}

} // namespace ToolkitUtil