#pragma once
//------------------------------------------------------------------------------
/**
    @class Toolkit::TextureBatcherApp.

    Application class for the texturebatcher3 tool.
    
    @todo WaitForKey not implemented!
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/distributedtools/distributedtoolkitapp.h"
#include "toolkitutil/texutil/textureconverter.h"

//------------------------------------------------------------------------------
namespace Toolkit
{
class TextureBatcherApp : public DistributedTools::DistributedToolkitApp
{
public:
    /// perform texture specific actions
    virtual void DoWork();

private:
    /// parse command line arguments
    virtual bool ParseCmdLineArgs();
    /// setup project info object
    virtual bool SetupProjectInfo();
    /// print help text
    virtual void ShowHelp();
    /// create a string list of all files that are processed by this application
    virtual Util::Array<Util::String> CreateFileList();
    /// return a list of files in this directory
    Util::Array<Util::String> GetFileListFromDirectory(const Util::String& directory);
    /// return platform dependent file extension
    Util::String GetDstTextureFileExtension();
    /// check if conversion is required (build dstPath from srcPath)
    bool NeedsConversion(const Util::String& srcPath);
    /// check if conversion is required (use explicit dstPath)
    bool NeedsConversion(const Util::String& srcPath, const Util::String& dstPath);

    ToolkitUtil::TextureConverter textureConverter;
    Util::String category;
    Util::String texture;
};

} // namespace Toolkit
//------------------------------------------------------------------------------
