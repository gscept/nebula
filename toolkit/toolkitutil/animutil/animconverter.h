#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AnimConverter
    
    Wrap animation conversion and processing stuff.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/platform.h"
#include "toolkit-common/logger.h"
#include "animutil/animbuilder.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class AnimConverter
{
public:
    /// constructor
    AnimConverter();
    /// destructor
    ~AnimConverter();

    /// set target platform
    void SetPlatform(Platform::Code platform);
    /// set source directory
    void SetSrcDir(const Util::String& srcDir);
    /// get source directory
    const Util::String& GetSrcDir() const;
    /// set destination directory
    void SetDstDir(const Util::String& dstDir);
    /// get destination directory
    const Util::String& GetDstDir() const;
    /// set force flag
    void SetForceFlag(bool b);
    /// set flag to create anim-driven-motion data for characters
    void SetAnimDrivenMotionFlag(bool b);

    /// setup the anim converter
    void Setup(Logger& logger);
    /// discard the anim converter
    void Discard();
    /// return true if the object has been setup
    bool IsValid() const;
    /// process all animations
    bool ProcessAll();
    /// process all animations in a category
    bool ProcessCategory(const Util::String& categoryName);
    /// process a single animation
    bool ProcessAnimation(const Util::String& categoryName, const Util::String& animFileName);
    /// process a list of files
    bool ProcessFiles(const Util::Array<Util::String>& files);

private:
    /// detect whether this is a character category
    bool IsCharacterCategory(const Util::String& categoryName);
    /// detect whether this is a bundled character animation
    bool IsBundledCharacterAnimationFile(const Util::String& categoryName, const Util::String& animFileName);
    /// detect whether this is a bundled variation file
    bool IsBundledCharacterVariationFile(const Util::String& categoryName, const Util::String& animFileName);
    /// extract the source animation clip directory for bundled character anim or variation clips
    Util::String ExtractClipDirectory(const Util::String& categoryName, const Util::String& animFileName);
    /// build path to Nax2 source file
    Util::String BuildSrcPath(const Util::String& categoryName, const Util::String& animFileName);
    /// build path to Nax3 dest file
    Util::String BuildDstPath(const Util::String& categoryName, const Util::String& animFileName);
    /// load NAX2 animation into an anim builder object
    bool LoadNax2Animation(const Util::String& categoryName, const Util::String& animFileName);
    /// save NAX3 animation from anim builder object
    bool SaveNax3Animation(const Util::String& categoryName, const Util::String& animFileName);
    /// check if conversion is needed 
    bool NeedsConversion(const Util::String& srcFileName, const Util::String& dstFileName);

    Platform::Code platform;
    Logger* logger;
    Util::String srcDir;
    Util::String dstDir;
    AnimBuilder animBuilder;
    bool forceFlag;
    bool animDrivenMotionFlag;
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimConverter::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimConverter::SetPlatform(Platform::Code p)
{
    this->platform = p;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimConverter::SetSrcDir(const Util::String& str)
{
    this->srcDir = str;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AnimConverter::GetSrcDir() const
{
    return this->srcDir;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimConverter::SetDstDir(const Util::String& str)
{
    this->dstDir = str;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AnimConverter::GetDstDir() const
{
    return this->dstDir;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimConverter::SetForceFlag(bool b)
{
    this->forceFlag = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimConverter::SetAnimDrivenMotionFlag(bool b)
{
    this->animDrivenMotionFlag = b;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
