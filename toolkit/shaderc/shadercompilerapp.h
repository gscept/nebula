#pragma once
//------------------------------------------------------------------------------
/**
    @class Toolkit::ShaderCompilerApp

    Application class for the shaderc tool.
        
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/toolkitapp.h"
#include "singleshadercompiler.h"

//------------------------------------------------------------------------------
namespace Toolkit
{
class ShaderCompilerApp : public ToolkitUtil::ToolkitApp
{
public:
    /// run the application
    virtual void Run();

private:
    /// parse command line arguments
    virtual bool ParseCmdLineArgs();    

    ToolkitUtil::SingleShaderCompiler shaderCompiler;
    Util::String src;
    Util::String type;
};

} // namespace Toolkit
//------------------------------------------------------------------------------

    