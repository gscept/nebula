#pragma once
//------------------------------------------------------------------------------
/**
    @class Toolkit::TexGenApp.

    Application class for the texture generator tool.
    
    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/toolkitapp.h"

//------------------------------------------------------------------------------
namespace Toolkit
{
class TexGenApp : public ToolkitUtil::ToolkitApp
{
public:
    TexGenApp();
    /// perform texture specific actions
    virtual void Run() override;

    /// parse cmd line args
    virtual bool ParseCmdLineArgs() override;
private:
    /// print help text
    virtual void ShowHelp() override;

    Util::String outputFile;
    Util::String inputFile;
    Util::String outputBGRA[4];
    float BGRA[4];
    int outputWidth;
    int outputHeight;
    int outputChannels;
};

} // namespace Toolkit
//------------------------------------------------------------------------------
