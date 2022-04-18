#pragma once
//------------------------------------------------------------------------------
/**
    @class Toolkit::ShaderBatcherApp

    Application class for the shaderbatcher3 tool.
    
    @todo WaitForKey not implemented!

    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/toolkitapp.h"
#include "shadercompiler.h"

//------------------------------------------------------------------------------
namespace Toolkit
{
class ShaderBatcherApp : public ToolkitUtil::ToolkitApp
{
public:
    /// run the application
    virtual void Run();

private:
    /// parse command line arguments
    virtual bool ParseCmdLineArgs();
    /// setup project info object
    virtual bool SetupProjectInfo();
    /// print help text
    virtual void ShowHelp();

    ToolkitUtil::ShaderCompiler shaderCompiler;
};

} // namespace Toolkit
//------------------------------------------------------------------------------

    