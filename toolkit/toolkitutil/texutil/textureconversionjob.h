#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::TextureConversionJob

    Base class for platform specific texture conversions. Override the
    conversion process for different platforms in subclasses.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "toolkitutil/texutil/textureattrtable.h"
#include "toolkit-common/logger.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class TextureConversionJob
{
public:
    /// Constructor
    TextureConversionJob();
    
    /// set texture attribute table path
    void SetTexAttrTable(const TextureAttrTable* table);
    /// set pointer to a valid logger object
    void SetLogger(Logger* logger);
    /// set conversion tool path
    void SetToolPath(const Util::String & path);
    /// set force flag
    void SetForceFlag(bool b);
    /// set quiet flag
    void SetQuietFlag(bool b);
    /// set temp directory
    void SetTmpDir(const Util::String& dir);
    /// set source path
    void SetSrcPath(const Util::String& path);
    /// set destination path
    void SetDstPath(const Util::String& path);
    
    /// perform the texture conversion
    virtual bool Convert();

protected:
    /// prepares conversion process
    virtual bool PrepareConversion(const Util::String& srcPath, const Util::String& dstPath);
    /// checks if conversion is required
    virtual bool NeedsConversion(const Util::String& srcPath, const Util::String& dstPath);
    /// set destination file extension (call from subclass constructor)
    void SetDstFileExtension(const Util::String & ext);
    /// copy conversion result from temp to dst path
    bool CopyResult();

    const TextureAttrTable* textureAttrTable;
    Logger* logger;
    TextureAttrs textureAttrs;
    Util::String toolPath;
    Util::String dstFileExt;

    Util::String tmpDir;
    Util::String srcPath;
    Util::String dstPath;
    Util::String tmpPath;
    bool force;
    bool quiet;
    bool neverCopy;


};

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConversionJob::SetLogger(Logger* l)
{
    this->logger = l;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConversionJob::SetTexAttrTable(const TextureAttrTable* table)
{
    this->textureAttrTable = table;
}

//------------------------------------------------------------------------------
/**
    Set conversion tool path
*/
inline void
TextureConversionJob::SetToolPath(const Util::String & path)
{
    this->toolPath = path;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConversionJob::SetForceFlag(bool b)
{
    this->force = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConversionJob::SetQuietFlag(bool b)
{
    this->quiet = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConversionJob::SetTmpDir(const Util::String & dir)
{
    this->tmpDir = dir;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConversionJob::SetSrcPath(const Util::String& path)
{
    this->srcPath = path;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConversionJob::SetDstPath(const Util::String& path)
{
    this->dstPath = path;
    this->dstPath.StripFileExtension();
    this->dstPath.Append(".");
    this->dstPath.Append(this->dstFileExt);
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConversionJob::SetDstFileExtension(const Util::String& ext)
{
    this->dstFileExt = ext;
}

} // namespace ToolkitUtil