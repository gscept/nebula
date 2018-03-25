//------------------------------------------------------------------------------
//  textureconverter.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "textureconverter.h"
#include "io/ioserver.h"
#include "io/textwriter.h"
#include "io/xmlwriter.h"
#include "toolkitutil/texutil/imageconverter.h"
#include "util/guid.h"
#include "toolkitutil/texutil/win32textureconversionjob.h"
#include "toolkitutil/texutil/nvtttextureconversionjob.h"
#include "timing/timer.h"


namespace ToolkitUtil
{
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
TextureConverter::TextureConverter() :
    logger(0),
    platform(Platform::Win32),
    force(false),
    quiet(false),
    valid(false),
    maxParallelJobs(1),
    textureAttrTable(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TextureConverter::~TextureConverter()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
TextureConverter::Setup(Logger& logger)
{
    n_assert(!this->IsValid());
    n_assert(0 == this->logger);

    this->logger = &logger;

    // setup texture attribute pointer (use external table, or owned table)
    if (0 == this->textureAttrTable)
    {
        // setup our own texture attributes table
        n_assert(this->texAttrTablePath.IsValid());
        if (!this->ownedTexAttrTable.Setup(this->texAttrTablePath))
        {
            logger.Error("failed to open '%s'!", this->texAttrTablePath.AsCharPtr());
            return false;
        }
        this->textureAttrTable = &this->ownedTexAttrTable;
    }
    else
    {
        // use externally provided texture attribute table
        n_assert(this->textureAttrTable->IsValid());
    }
    this->valid = true;

    // create a temporary directory
    IoServer::Instance()->CreateDirectory("temp:textureconverter");

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureConverter::Discard()
{
    n_assert(this->IsValid());
    this->valid = false;
    this->logger = 0;

    // if no external texture attribute table provided, discard our own
    if (&this->ownedTexAttrTable ==  this->textureAttrTable)
    {
        this->ownedTexAttrTable.Discard();
        this->textureAttrTable = 0;
    }

    // deletes the temporary directory
    if (IoServer::Instance()->DirectoryExists("temp:textureconverter"))
    {
        IoServer::Instance()->DeleteDirectory("temp:textureconverter");
    }
}

//------------------------------------------------------------------------------
/**
    Convert all textures in a given file list.
*/
bool
TextureConverter::ConvertFiles(const Util::Array<Util::String>& files)
{
    bool success = true;

    // create temp directory from guid. so that other jobs won't interfere
    Guid guid;
    guid.Generate();
    String tmpDir;
    tmpDir.Format("%s/%s", "temp:textureconverter", guid.AsString().AsCharPtr());

    // convert each texture in file list
    IndexT index;
    for(index = 0; index < files.Size(); index++)
    {
        String dirName = files[index].ExtractLastDirName();
        if (files[index].CheckFileExtension("tga") ||
            files[index].CheckFileExtension("bmp") ||
            files[index].CheckFileExtension("dds") ||
            files[index].CheckFileExtension("psd") ||
			files[index].CheckFileExtension("png") ||
			files[index].CheckFileExtension("jpg"))
        {
            success = this->ConvertTexture(files[index], tmpDir);
        }
    }
    // remove created temp directory of this job
    if (IoServer::Instance()->DirectoryExists(tmpDir))
    {
        IoServer::Instance()->DeleteDirectory(tmpDir);
    }
    return success;
}

//------------------------------------------------------------------------------
/**
    Convert a source texture defined by an absolute path into a destination
    texture defined by category name and texture name. The destination texture
    name may not have a file extension! This method will just branch to
    different platform-specific jobs based on the selected target platform.
*/
bool
TextureConverter::ConvertTexture(const String& srcTexPath, const String& tmpDir)
{
    n_assert(this->IsValid());
    n_assert(srcTexPath.IsValid());
    n_assert(tmpDir.IsValid());
    n_assert(this->dstDir.IsValid());
    n_assert(0 != this->logger);

    // extract texture category and filename from path (last 2 components)
    // NOTE: the dstTexPath will contain the source file extension, which 
    // is bad design and very confusing - the TextureConversionJob will
    // replace with the platform-specific correct file extension in its
    // Start method!
    Array<String> tokens = srcTexPath.Tokenize(":/");
    n_assert(tokens.Size() >= 3);
    String texCategory = tokens[tokens.Size() - 2];
    String texFilename = tokens[tokens.Size() - 1];
    String dstTexPath;
    dstTexPath.Format("%s/%s/%s", this->dstDir.AsCharPtr(), texCategory.AsCharPtr(), texFilename.AsCharPtr());
    dstTexPath.StripFileExtension();

	n_printf("converting texture: %s\n", srcTexPath.AsCharPtr());

    // select conversion method based on target platform
#ifndef USE_NVTT
	Win32TextureConversionJob job;
	job.SetLogger(this->logger);
	job.SetSrcPath(srcTexPath);
	job.SetDstPath(dstTexPath);
	job.SetTmpDir(tmpDir);
	job.SetTexAttrTable(this->textureAttrTable);
	job.SetToolPath(this->toolPath);
	job.SetForceFlag(this->force);
	job.SetQuietFlag(this->quiet);
	job.Convert();
#else
	NVTTTextureConversionJob job;
	job.SetLogger(this->logger);
	job.SetSrcPath(srcTexPath);
	job.SetDstPath(dstTexPath);
	job.SetTmpDir(tmpDir);
	job.SetTexAttrTable(this->textureAttrTable);
	job.SetToolPath(this->toolPath);
	job.SetForceFlag(this->force);
	job.SetQuietFlag(this->quiet);
	job.Convert();
#endif
	
	if (this->platform != Platform::Win32 && this->platform != Platform::Linux) return false;
    else return true;
}

} // namespace ToolkitUtil