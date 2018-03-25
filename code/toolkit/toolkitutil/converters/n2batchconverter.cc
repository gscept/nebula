//------------------------------------------------------------------------------
//  n2batchconverter.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "n2batchconverter.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace ToolkitUtil;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
N2BatchConverter::N2BatchConverter() :
    platform(Platform::Win32),
    force(false),
    verbose(true),
    isValid(false),
    logger(0),
    texAttrTable(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
N2BatchConverter::~N2BatchConverter()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
    NOTE: ProjectInfo must be set to the right platform already!
*/
bool
N2BatchConverter::CheckRequiredProjectInfoAttrs(const ProjectInfo& projInfo)
{
    if (!projInfo.HasAttr("DstDir")) return false;
    if (!projInfo.HasAttr("SrcDir")) return false;
    if (!projInfo.HasAttr("IntDir")) return false;
    if (!projInfo.HasAttr("N2ConverterSrcDir")) return false;
    if (!projInfo.HasAttr("N2ConverterDstDir")) return false;
    if (!projInfo.HasAttr("TextureTool")) return false;
    if (!projInfo.HasAttr("TextureAttrTable")) return false;
    if (!projInfo.HasAttr("TextureSrcDir")) return false;
    if (!projInfo.HasAttr("TextureSrcDir")) return false;
    if (!projInfo.HasAttr("TextureDstDir")) return false;
    if (!projInfo.HasAttr("AnimSrcDir")) return false;
    if (!projInfo.HasAttr("AnimDstDir")) return false;    
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
N2BatchConverter::Setup(Logger& logger, const ToolkitUtil::ProjectInfo& projInfo)
{
    n_assert(!this->IsValid());
    n_assert(0 == this->logger);
    this->isValid = true;
    this->logger = &logger;

    // setup tools assigns
    AssignRegistry::Instance()->SetAssign(Assign("src", projInfo.GetAttr("SrcDir")));
    AssignRegistry::Instance()->SetAssign(Assign("dst", projInfo.GetAttr("DstDir")));
    AssignRegistry::Instance()->SetAssign(Assign("int", projInfo.GetAttr("IntDir")));

    // setup N2 converter
    this->n2Converter.SetPlatform(this->platform);
    this->n2Converter.SetVerbose(this->verbose);
    this->n2Converter.SetForceFlag(this->force);
    this->n2Converter.SetSrcDir(projInfo.GetAttr("N2ConverterSrcDir"));
    this->n2Converter.SetDstDir(projInfo.GetAttr("N2ConverterDstDir"));
    this->n2Converter.Setup();

    // setup texture converter
    this->texConverter.SetPlatform(this->platform);
    this->texConverter.SetQuietFlag(!this->verbose);
    this->texConverter.SetForceFlag(this->force);
    this->texConverter.SetToolPath(projInfo.GetPathAttr("TextureTool"));
    this->texConverter.SetSrcDir(projInfo.GetAttr("TextureSrcDir"));
    this->texConverter.SetDstDir(projInfo.GetAttr("TextureDstDir"));
    this->texConverter.SetExternalTextureAttrTable(this->texAttrTable);    
    this->texConverter.Setup(logger);

    // setup anim converter
    this->animConverter.SetPlatform(this->platform);
    this->animConverter.SetForceFlag(this->force);
    this->animConverter.SetSrcDir(projInfo.GetAttr("AnimSrcDir"));
    this->animConverter.SetDstDir(projInfo.GetAttr("AnimDstDir"));
    this->animConverter.Setup(logger);
}

//------------------------------------------------------------------------------
/**
*/
void
N2BatchConverter::Discard()
{
    n_assert(this->IsValid());
    n_assert(0 != this->logger);
    this->texConverter.Discard();
    this->animConverter.Discard();
    this->n2Converter.Discard();
    this->logger = 0;
    this->isValid = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
N2BatchConverter::ConvertAll()
{
    n_assert(this->IsValid());

    // convert .n2 to .n3 files
    if (!this->n2Converter.ConvertAll(*this->logger))
    {
        this->logger->Error("N2BatchConverter::ConvertAll(): N2Converter::ConvertAll() failed!\n");
        return false;
    }

    // convert the resources used by the N2>N3 conversion
    if (!this->ConvertResources(this->n2Converter.GetUsedResources()))
    {
        this->logger->Error("N2BatchConverter::ConvertAll(): Failed to convert resources!\n");
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
N2BatchConverter::ConvertCategory(const String& category)
{
    n_assert(this->IsValid());

    // convert .n2 to .n3 files
    if (!this->n2Converter.ConvertCategory(*this->logger, category))
    {
        this->logger->Error("N2BatchConverter::ConvertCategory(%s): N2Converter::ConvertCategory() failed!\n", category.AsCharPtr());
        return false;
    }

    // convert the resources used by the N2>N3 conversion
    if (!this->ConvertResources(this->n2Converter.GetUsedResources()))
    {
        this->logger->Error("N2BatchConverter::ConvertCategory(%s): Failed to convert resources!\n", category.AsCharPtr());
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
N2BatchConverter::ConvertFile(const String& category, const String& srcFile)
{
    n_assert(this->IsValid());

    // convert .n2 to .n3 files
    if (!this->n2Converter.ConvertFile(*this->logger, category, srcFile))
    {
        this->logger->Error("N2BatchConverter::ConvertFile(%s, %s): N2Converter::ConvertFile(%s) failed!\n", category.AsCharPtr(), srcFile.AsCharPtr());
        return false;
    }

    // convert the resources used by the N2>N3 conversion
    if (!this->ConvertResources(this->n2Converter.GetUsedResources()))
    {
        this->logger->Error("N2BatchConverter::ConvertFile(%s, %s): Failed to convert resources!\n", category.AsCharPtr(), srcFile.AsCharPtr());
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
N2BatchConverter::ConvertResources(const Array<String>& resIds)
{
    n_assert(this->IsValid());

    // split resources into resource type arrays
    String texPattern("tex:*");
    String aniPattern("ani:*");
    String mshPattern("msh:*");

    Array<String> texPaths;
    Array<String> animPaths;
    texPaths.Reserve(1024);
    animPaths.Reserve(1024);
    IndexT i;
    for (i = 0; i < resIds.Size(); i++)
    {
        const String& curResId = resIds[i];
        if (String::MatchPattern(curResId, texPattern))
        {
            String texPath = this->BuildSourceTexturePath(curResId);
            if (texPath.IsValid())
            {
                texPaths.Append(texPath);
            }
        }
        else if (String::MatchPattern(curResId, aniPattern))
        {
            String animPath = this->BuildSourceAnimPath(curResId);
            if (animPath.IsValid())
            {
                animPaths.Append(animPath);
            }
        }
        else if (String::MatchPattern(curResId, mshPattern))
        {
            // ignore mesh files, they are already at the right place
        }
        else
        {
            this->logger->Warning("IGNORING INVALID RESOURCE PATH '%s'!\n", curResId.AsCharPtr());
        }
    }

    // convert textures
    if (!this->texConverter.ConvertFiles(texPaths))
    {
        this->logger->Warning("ERRORS DURING TEXTURE CONVERSION!\n");
    }

    // convert animations
    if (!this->animConverter.ProcessFiles(animPaths))
    {
        this->logger->Warning("ERRORS DURING ANIMATION CONVERSION!\n");
    }

    return true;
}

//------------------------------------------------------------------------------
/**
    Takes a target texture path (e.g. "tex:category/texture.dss") and 
    converts it to its source texture (.psd or .tga).
*/
String
N2BatchConverter::BuildSourceTexturePath(const String& resId) const
{
    // extract category and texture file name
    Array<String> tokens = resId.Tokenize(":/");
    n_assert(tokens.Size() >= 3);
    String texFileName = tokens[tokens.Size() - 1];
    String texCategoryName = tokens[tokens.Size() - 2];
    texFileName.StripFileExtension();

    // need to check whether this was a PSD, TGA or DDS file
    String tgaPath, psdPath, ddsPath;
    tgaPath.Format("%s/%s/%s.tga", this->texConverter.GetSrcDir().AsCharPtr(), texCategoryName.AsCharPtr(), texFileName.AsCharPtr());
    psdPath.Format("%s/%s/%s.psd", this->texConverter.GetSrcDir().AsCharPtr(), texCategoryName.AsCharPtr(), texFileName.AsCharPtr());
    ddsPath.Format("%s/%s/%s.dds", this->texConverter.GetSrcDir().AsCharPtr(), texCategoryName.AsCharPtr(), texFileName.AsCharPtr());

    IoServer* ioServer = IoServer::Instance();
    if (ioServer->FileExists(psdPath))
    {
        return psdPath;
    }
    else if (ioServer->FileExists(tgaPath))
    {
        return tgaPath;
    }
    else if (ioServer->FileExists(ddsPath))
    {
        return ddsPath;
    }
    else
    {
        this->logger->Warning("Could not find source texture for '%s' (psd, tga or dds)\n", resId.AsCharPtr());
        return "";
    }
}

//------------------------------------------------------------------------------
/**
    Builds the source nax2 file path from the destination nax3 path.
*/
String
N2BatchConverter::BuildSourceAnimPath(const String& resId) const
{
    // extract category and anim file name
    Array<String> tokens = resId.Tokenize(":/");
    n_assert(tokens.Size() >= 3);
    String animFileName = tokens[tokens.Size() - 1];
    String animCategoryName = tokens[tokens.Size() - 2];
    animFileName.StripFileExtension();

    // build source path
    String nax2Path;
    nax2Path.Format("%s/%s/%s.nax2", this->animConverter.GetSrcDir().AsCharPtr(), animCategoryName.AsCharPtr(), animFileName.AsCharPtr());
    if (IoServer::Instance()->FileExists(nax2Path))
    {
        return nax2Path;
    }
    else
    {
        return "";
    }
}

} // namespace ToolkitUtil
