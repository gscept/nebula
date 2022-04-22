//------------------------------------------------------------------------------
//  shadercompilerapp.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "shadercompilerapp.h"
#include "timing/time.h"

namespace Toolkit
{
using namespace ToolkitUtil;

//------------------------------------------------------------------------------
/**
*/
bool
ShaderCompilerApp::ParseCmdLineArgs()
{
    if (ToolkitApp::ParseCmdLineArgs())
    {   
        if (!this->args.HasArg("-t") || !this->args.HasArg("-i") || !this->args.HasArg("-o"))
        {
            n_printf("shaderc error: No compile type, input file or output specified\n");
            return false;
        }

        this->shaderCompiler.SetDebugFlag(this->args.GetBoolFlag("-debug"));
        this->shaderCompiler.SetDstDir(this->args.GetString("-o"));
        this->shaderCompiler.SetHeaderDir(this->args.GetString("-h"));

        // find include dir args
        for (SizeT i = 0; i < this->args.GetNumArgs(); i++)
        {
            if (this->args.GetStringAtIndex(i) == "-I" && i+1 < this->args.GetNumArgs())
            {
                this->shaderCompiler.AddIncludeDir(this->args.GetStringAtIndex(i + 1));
            }
        }
        
        this->src = this->args.GetString("-i");
        this->type = this->args.GetString("-t");        
    }
    return true;
}



//------------------------------------------------------------------------------
/**
*/
void
ShaderCompilerApp::Run()
{
    this->SetReturnCode(-1);

    // parse command line args
    if(!this->ParseCmdLineArgs())
    {
        return;
    }
    bool success = false;
    if (this->type == "frame")
    {
        success = this->shaderCompiler.CompileFrameShader(this->src);
    }
    else if (this->type == "material")
    {
        success = this->shaderCompiler.CompileMaterial(this->src);
    }
    else if (this->type == "shader")
    {        
        if(this->args.HasArg("-M"))
        {
            success = this->shaderCompiler.CreateDependencies(this->src);
        }
        else
        {
            success = this->shaderCompiler.CompileShader(this->src);
        }
    }
    else
    {
        n_printf("[shaderc] error: unknown type: %s\n", this->type.AsCharPtr());
    }

    if (success)
    {
        this->SetReturnCode(0);
    }    
}

} // namespace Toolkit

