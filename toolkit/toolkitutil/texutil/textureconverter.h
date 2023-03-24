#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::TextureConverter
    
    Wraps texture conversion process for all supported target platforms.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/platform.h"
#include "io/uri.h"
#include "util/string.h"
#include "toolkitutil/texutil/textureattrtable.h"
#include "toolkit-common/applauncher.h"
#include "toolkit-common/logger.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class TextureConverter
{
public:
    /// constructor
    TextureConverter();
    /// destructor
    ~TextureConverter();

    /// set target platform
    void SetPlatform(Platform::Code platform);
    /// set texture attribute table path
    void SetTexAttrTablePath(const Util::String& path);
    /// set source directory
    void SetSrcDir(const Util::String& srcDir);
    /// get source directory
    const Util::String& GetSrcDir() const;
    /// set destination directory
    void SetDstDir(const Util::String& dstDir);
    /// get destination directory
    const Util::String& GetDstDir() const;
    /// get destination directory
    Util::String& GetDstDir();
    /// set path to external converter tool (platform specific)
    void SetToolPath(const Util::String& toolPath);
    /// set force flag
    void SetForceFlag(bool b);
    /// set quiet flag
    void SetQuietFlag(bool b);
    /// set optional external texture attribute table (so it doesn't need to be loaded during setup)
    void SetTextureAttrTable(TextureAttrTable&& extTexAttrTable);

    /// set max parallel job count
    void SetMaxParallelJobs(int count);
    /// get max parallel job count
    int GetMaxParallelJobs();

    /// Add entry
    void AddAttributeEntry(const Util::String& file, const TextureAttrs& attrs);

    /// setup the texture converter, read the texture attributes table
    bool Setup(Logger& logger);
    /// discard the texture converter
    void Discard();
    /// return true if the object has been setup
    bool IsValid() const;
    /// convert all textures from given file list
    bool ConvertFiles(const Util::Array<Util::String>& files);
    /// convert a texture from a given path
    bool ConvertTexture(const Util::String& srcTexPath, const Util::String& dstDir, const Util::String& tmpDir);
    /// convert a cubemap folder from a given path
    bool ConvertCubemap(const Util::String& srcTexPath, const Util::String& dstDir, const Util::String& tmpDir);

private:
    Logger* logger;
    Platform::Code platform;
    Util::String texAttrTablePath;
    Util::String toolPath;
    bool force;
    bool quiet;
    bool valid;
    TextureAttrTable textureAttrTable;
    int maxParallelJobs;
};

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConverter::SetTextureAttrTable(TextureAttrTable&& extTexAttrTable)
{
    this->textureAttrTable = extTexAttrTable;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
TextureConverter::IsValid() const
{
    return this->valid;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConverter::SetPlatform(Platform::Code p)
{
    this->platform = p;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConverter::SetTexAttrTablePath(const Util::String& path)
{
    this->texAttrTablePath = path;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConverter::SetToolPath(const Util::String& p)
{
    this->toolPath = p;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConverter::SetForceFlag(bool b)
{
    this->force = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConverter::SetQuietFlag(bool b)
{
    this->quiet = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureConverter::SetMaxParallelJobs(int count)
{
    n_assert(count > 0);
    this->maxParallelJobs = count;
}

//------------------------------------------------------------------------------
/**
*/
inline int
TextureConverter::GetMaxParallelJobs()
{
    return this->maxParallelJobs;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------

