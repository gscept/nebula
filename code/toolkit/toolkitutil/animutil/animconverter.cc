//------------------------------------------------------------------------------
//  animconverter.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/animutil/animconverter.h"
#include "toolkitutil/animutil/animbuilderloader.h"
#include "toolkitutil/animutil/animbuildersaver.h"
#include "io/ioserver.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
AnimConverter::AnimConverter() :    
    platform(Platform::Win32),
    forceFlag(false),
    animDrivenMotionFlag(false),
    isValid(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AnimConverter::~AnimConverter()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AnimConverter::Setup(Logger& logger)
{
    n_assert(!this->IsValid());
    this->isValid = true;
    this->logger = &logger;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimConverter::Discard()
{
    n_assert(this->IsValid());
    this->logger = 0;
    this->isValid = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimConverter::ProcessAll()
{
    n_assert(this->IsValid());
    bool success = true;
    Array<String> categories = IoServer::Instance()->ListDirectories(this->srcDir, "*");
    IndexT i;
    for (i = 0; i < categories.Size(); i++)
    {
        // ignore revision control system dirs
        if ((categories[i] != "CVS") && (categories[i] != ".svn"))
        {
            success &= this->ProcessCategory(categories[i]);
        }
    }
    return success;
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimConverter::ProcessCategory(const String& categoryName)
{
    n_assert(this->IsValid());
    n_assert(categoryName.IsValid());
    bool success = true;

    // list files in category
    String catDir;
    catDir.Format("%s/%s", this->srcDir.AsCharPtr(), categoryName.AsCharPtr());
    Array<String> files = IoServer::Instance()->ListFiles(catDir, "*.nax2");

    IndexT fileIndex;
    for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
    {
        success &= this->ProcessAnimation(categoryName, files[fileIndex]);
    }
    return success;
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimConverter::ProcessFiles(const Array<String>& files)
{
    n_assert(this->IsValid());

    bool result = true;
    IndexT i;
    for (i = 0; i < files.Size(); i++)
    {
        // extract category and filename
        const String& curPath = files[i];
        Array<String> tokens = curPath.Tokenize(":/");
        n_assert(tokens.Size() >= 3);
        const String& catName = tokens[tokens.Size() - 2];
        const String& fileName = tokens[tokens.Size() - 1];
        if (!this->ProcessAnimation(catName, fileName))
        {
            result = false;
        }
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimConverter::ProcessAnimation(const String& categoryName, const String& animFileName)
{
    n_assert(this->IsValid());
    n_assert(categoryName.IsValid());
    n_assert(animFileName.IsValid());

    n_printf("-> processing '%s/%s'...", categoryName.AsCharPtr(), animFileName.AsCharPtr());

    this->animBuilder.Clear();
    String srcPath = this->BuildSrcPath(categoryName, animFileName);
    String dstPath = this->BuildDstPath(categoryName, animFileName);
    if (this->NeedsConversion(srcPath, dstPath))
    {
        // load nax2 anim file
        if (!this->LoadNax2Animation(categoryName, animFileName))
        {
            n_printf("FAILED ON LOAD\n");
            return false;
        }

        // generate anim-driven-motion data when requested
        // (currently, only for characters)
        if (this->animDrivenMotionFlag && this->IsBundledCharacterAnimationFile(categoryName, animFileName))
        {
            // build velocity curves from translation curves
            this->animBuilder.BuildVelocityCurves();

            // remove the last key, clips for anim-driven-motion have an 
            // additional key at the end which is identical with the first key
            this->animBuilder.TrimEnd(1);
        }

        // save nax3 anim file
        if (!this->SaveNax3Animation(categoryName, animFileName))
        {
            n_printf("FAILED ON SAVE\n");
            return false;
        }
    }
    n_printf("ok\n");
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimConverter::IsCharacterCategory(const String& categoryName)
{
    return categoryName == "characters";
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimConverter::IsBundledCharacterAnimationFile(const String& categoryName, const String& animFileName)
{
    return this->IsCharacterCategory(categoryName) && String::MatchPattern(animFileName, "*_animations.*");
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimConverter::IsBundledCharacterVariationFile(const String& categoryName, const String& animFileName)
{
    return this->IsCharacterCategory(categoryName) && String::MatchPattern(animFileName, "*_variations.*");
}

//------------------------------------------------------------------------------
/**
*/
String
AnimConverter::BuildSrcPath(const String& categoryName, const String& animFileName)
{
    String path;
    path.Format("%s/%s/%s", this->srcDir.AsCharPtr(), categoryName.AsCharPtr(), animFileName.AsCharPtr());
    return path;
}

//------------------------------------------------------------------------------
/**
*/
String
AnimConverter::BuildDstPath(const String& categoryName, const String& animFileName)
{
    String nax3FileName = animFileName;
    nax3FileName.StripFileExtension();
    nax3FileName.Append(".nax3");
    String path;
    path.Format("%s/%s/%s", this->dstDir.AsCharPtr(), categoryName.AsCharPtr(), nax3FileName.AsCharPtr());
    return path;
}

//------------------------------------------------------------------------------
/**
*/
String
AnimConverter::ExtractClipDirectory(const String& categoryName, const String& animFileName)
{
    n_assert(this->IsBundledCharacterAnimationFile(categoryName, animFileName) ||
             this->IsBundledCharacterVariationFile(categoryName, animFileName));
    String charName = animFileName;
    charName.StripFileExtension();
    String pathName;
    if (this->IsBundledCharacterAnimationFile(categoryName, animFileName))
    {
        charName.TerminateAtIndex(charName.FindStringIndex("_animations"));
        pathName.Format("%s/%s/%s/animations", this->srcDir.AsCharPtr(), categoryName.AsCharPtr(), charName.AsCharPtr());
    }
    else
    {
        charName.TerminateAtIndex(charName.FindStringIndex("_variations"));
        pathName.Format("%s/%s/%s/variations", this->srcDir.AsCharPtr(), categoryName.AsCharPtr(), charName.AsCharPtr());
    }
    return pathName;
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimConverter::LoadNax2Animation(const String& categoryName, const String& animFileName)
{
    // since NAX2 doesn't contain the clip name(s) we need to obtain them
    // manually, this involves listing the original anim clips if this
    // is a bundled character animation or variation file
    bool autoGenerateClipNames;
    Array<String> clipNames;
    if (this->IsCharacterCategory(categoryName))
    {
        autoGenerateClipNames = false;
        String clipDir = this->ExtractClipDirectory(categoryName, animFileName);
        clipNames = IoServer::Instance()->ListFiles(clipDir, "*.nax2");
        clipNames.Sort();
        IndexT i;
        for (i = 0; i < clipNames.Size(); i++)
        {
            clipNames[i].StripFileExtension();
        }
    }
    else
    {
        // if not a character animation, setup dummy clip names
        autoGenerateClipNames = true;
    }

    // use an AnimBuilderLoader to actually load the file
    String path = this->BuildSrcPath(categoryName, animFileName);
    bool res = AnimBuilderLoader::LoadNax2(path, this->animBuilder, clipNames, autoGenerateClipNames);
    if (!res)
    {
        this->logger->Warning("Failed to load anim '%s'!\n", path.AsCharPtr());
    }        
    return res;
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimConverter::SaveNax3Animation(const String& categoryName, const String& animFileName)
{
    String path = this->BuildDstPath(categoryName, animFileName);
    bool res = AnimBuilderSaver::SaveNax3(path, this->animBuilder, this->platform);
    if (!res)
    {
        this->logger->Warning("Failed to save anim '%s'!\n", path.AsCharPtr());
    }
    return res;
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimConverter::NeedsConversion(const String& srcPath, const String& dstPath)
{
    // file time check overriden?
    if (this->forceFlag)
    {
        return true;
    }

    // otherwise check file times of src and dst file
    IoServer* ioServer = IoServer::Instance();
    if (ioServer->FileExists(dstPath))
    {
        FileTime srcFileTime = ioServer->GetFileWriteTime(srcPath);
        FileTime dstFileTime = ioServer->GetFileWriteTime(dstPath);
        if (dstFileTime > srcFileTime)
        {
            // dst file newer then src file, don't need to convert
            return false;
        }
    }

    // fallthrough: dst file doesn't exist, or it is older then the src file
    return true;
}

} // namespace ToolkitUtil
