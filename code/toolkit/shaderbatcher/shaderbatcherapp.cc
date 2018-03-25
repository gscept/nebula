//------------------------------------------------------------------------------
//  shaderbatcherapp.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "shaderbatcherapp.h"
#include "timing/time.h"

namespace Toolkit
{
using namespace ToolkitUtil;

//------------------------------------------------------------------------------
/**
*/
bool
ShaderBatcherApp::ParseCmdLineArgs()
{
    if (ToolkitApp::ParseCmdLineArgs())
    {
        this->shaderCompiler.SetForceFlag(this->args.GetBoolFlag("-force"));
        this->shaderCompiler.SetDebugFlag(this->args.GetBoolFlag("-debug"));
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
ShaderBatcherApp::SetupProjectInfo()
{
    if (ToolkitApp::SetupProjectInfo())
    {
        this->shaderCompiler.SetPlatform(this->platform);
        if (this->projectInfo.HasAttr("ShaderToolParams"))
        {
            this->shaderCompiler.SetAdditionalParams(this->projectInfo.GetAttr("ShaderToolParams"));
        }

        // setup required stuff in compiler
        this->shaderCompiler.SetLanguage(this->projectInfo.GetAttr("ShaderLanguage"));
        this->shaderCompiler.SetSrcShaderBaseDir(this->projectInfo.GetAttr("ShaderSrcDir"));
        this->shaderCompiler.SetDstShaderDir(this->projectInfo.GetAttr("ShaderDstDir"));
        this->shaderCompiler.SetSrcFrameShaderBaseDir(this->projectInfo.GetAttr("FrameShaderSrcDir"));
        this->shaderCompiler.SetDstFrameShaderDir(this->projectInfo.GetAttr("FrameShaderDstDir"));
        this->shaderCompiler.SetSrcMaterialBaseDir(this->projectInfo.GetAttr("MaterialsSrcDir"));
        this->shaderCompiler.SetDstMaterialsDir(this->projectInfo.GetAttr("MaterialsDstDir"));

        // setup custom stuff
        if (this->projectInfo.HasAttr("ShaderSrcCustomDir")) this->shaderCompiler.SetSrcShaderCustomDir(this->projectInfo.GetAttr("ShaderSrcCustomDir"));
        if (this->projectInfo.HasAttr("FrameShaderSrcCustomDir")) this->shaderCompiler.SetSrcFrameShaderCustomDir(this->projectInfo.GetAttr("FrameShaderSrcCustomDir"));
        if (this->projectInfo.HasAttr("MaterialsSrcCustomDir")) this->shaderCompiler.SetSrcMaterialCustomDir(this->projectInfo.GetAttr("MaterialsSrcCustomDir"));
        
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderBatcherApp::ShowHelp()
{
    n_printf("Nebula Trifid shader batch compiler.\n"
             "(C) Individual contributors, see AUTHORS file\n"
             "-help       -- display this help\n"
             "-platform   -- select platform (win32, linux)\n"
             "-waitforkey -- wait for key when complete\n"
             "-force      -- force recompile\n"
             "-debug      -- compile with debugging information\n");             
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderBatcherApp::Run()
{
    bool success = true;
    // parse command line args
    if (success && !this->ParseCmdLineArgs())
    {
        success = false;
    }

    // setup the project info object
    if (success && !this->SetupProjectInfo())
    {
        success = false;
    }

    // call the shader compiler tool
    if (success && !this->shaderCompiler.CompileShaders())
    {
        success = false;
        this->SetReturnCode(-1);
    }
    if (success && !this->shaderCompiler.CompileFrameShaders())
    {
        success = false;
        this->SetReturnCode(-1);
    }
    if (success && !this->shaderCompiler.CompileMaterials())
    {
        success = false;
        this->SetReturnCode(-1);
    }

    // wait for user input
    if (this->waitForKey)
    {
        n_printf("Press <Enter> to continue!\n");
        while (!IO::Console::Instance()->HasInput())
        {
            Timing::Sleep(0.01);
        }
    }
}

} // namespace Toolkit

