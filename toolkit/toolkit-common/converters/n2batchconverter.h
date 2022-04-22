#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::N2BatchConverter
    
    Converts one or several .n2 files into .n3 files, plus all required 
    resource files.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "toolkitutil/platform.h"
#include "toolkitutil/converters/n2converter.h"
#include "toolkitutil/texutil/textureconverter.h"
#include "toolkitutil/animutil/animconverter.h"
#include "toolkitutil/projectinfo.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class N2BatchConverter
{
public:
    /// constructor
    N2BatchConverter();
    /// destructor
    ~N2BatchConverter();
    
    /// set target platform
    void SetPlatform(Platform::Code platform);
    /// set force conversion flag (otherwise check timestamps)
    void SetForceFlag(bool b);
    /// set verbosity on
    void SetVerbose(bool b);
    /// set optional, external texture attribute table
    void SetExternalTextureAttrTable(const TextureAttrTable* extTexAttrTable);

    /// test if the project ProjectInfo object contains all required attributes
    static bool CheckRequiredProjectInfoAttrs(const ProjectInfo& projInfo);
    /// setup the object
    void Setup(Logger& logger, const ProjectInfo& projInfo);
    /// discard the object
    void Discard();
    /// return true if object is valid
    bool IsValid() const;

    /// convert a single N2 file
    bool ConvertFile(const Util::String& category, const Util::String& srcFile);
    /// convert all files in a category
    bool ConvertCategory(const Util::String& category);
    /// convert all files
    bool ConvertAll();
    
private:
    /// convert a set of resources (textures, meshes, anims)
    bool ConvertResources(const Util::Array<Util::String>& resIds);
    /// build source texture path from texture resource id
    Util::String BuildSourceTexturePath(const Util::String& resId) const;
    /// build source anim path from resource id
    Util::String BuildSourceAnimPath(const Util::String& resId) const;

    Platform::Code platform;
    bool force;
    bool verbose;
    bool isValid;
    Logger* logger;

    const TextureAttrTable* texAttrTable;
    N2Converter n2Converter;
    TextureConverter texConverter;
    AnimConverter animConverter;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
N2BatchConverter::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
inline void
N2BatchConverter::SetPlatform(Platform::Code p)
{
    this->platform = p;
}

//------------------------------------------------------------------------------
/**
*/
inline void
N2BatchConverter::SetForceFlag(bool b)
{
    this->force = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
N2BatchConverter::SetVerbose(bool b)
{
    this->verbose = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
N2BatchConverter::SetExternalTextureAttrTable(const TextureAttrTable* extTexAttrTable)
{
    this->texAttrTable = extTexAttrTable;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    