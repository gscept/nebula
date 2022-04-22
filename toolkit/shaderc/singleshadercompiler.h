#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::ShaderC
    
    Compiles a single shader.
    
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkit-common/platform.h"
#include "io/uri.h"
#include "util/string.h"

namespace ToolkitUtil
{
class SingleShaderCompiler
{
public:

    /// constructor
    SingleShaderCompiler();
    /// destructor
    ~SingleShaderCompiler();

    /// set target platform
    void SetPlatform(Platform::Code platform);
    /// sets the source language
    void SetLanguage(const Util::String& lang);
    /// set source directory for base shaders
    void AddIncludeDir(const Util::String& incDir);    
    /// set destination directory
    void SetDstDir(const Util::String& dstDir);
    /// set output header directory
    void SetHeaderDir(const Util::String& headerDir);
            
    /// set debugging flag
    void SetDebugFlag(bool b);
    /// set additional command line params
    void SetAdditionalParams(const Util::String& params);
    /// set quiet flag
    void SetQuietFlag(bool b);

    /// compile shader
    bool CompileShader(const Util::String& src);
    /// compile material
    bool CompileMaterial(const Util::String& src);
    /// compile frame shader
    bool CompileFrameShader(const Util::String& src);
    /// calculate include dependencies
    bool CreateDependencies(const Util::String& src);
    
private:
    
    /// compiles shaders for glsl
    bool CompileGLSL(const Util::String& src);
    /// compiles shaders for SPIRV
    bool CompileSPIRV(const Util::String& src);
    
    Platform::Code platform;    
    Util::String dstDir;    
    Util::String headerDir;
    Util::String language;  
    bool quiet;
    bool debug;
    Util::String additionalParams;
    Util::Array<Util::String> includeDirs;  
}; 

//------------------------------------------------------------------------------
/**
*/
inline void
SingleShaderCompiler::SetPlatform(Platform::Code p)
{
    this->platform = p;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
SingleShaderCompiler::SetLanguage( const Util::String& lang )
{
    this->language = lang;
}

///------------------------------------------------------------------------------
/**
*/
inline void
SingleShaderCompiler::SetDstDir(const Util::String& d)
{
    this->dstDir = d;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
SingleShaderCompiler::SetHeaderDir(const Util::String& headerDir)
{
    this->headerDir = headerDir;
}

//------------------------------------------------------------------------------
/**
*/
inline void
SingleShaderCompiler::AddIncludeDir(const Util::String& d)
{
    this->includeDirs.Append(d);
}


//------------------------------------------------------------------------------
/**
*/
inline void
SingleShaderCompiler::SetDebugFlag(bool b)
{
    this->debug = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
SingleShaderCompiler::SetAdditionalParams(const Util::String& p)
{
    this->additionalParams = p;
}

//------------------------------------------------------------------------------
/**
*/
inline void
SingleShaderCompiler::SetQuietFlag(bool b)
{
    this->quiet = b;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------