#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::ShaderCompiler
    
    Compiles DX FX files.
    
    (C) 2012 Gustav Sterbrant
*/
//------------------------------------------------------------------------------
#include "toolkit-common/platform.h"
#include "io/uri.h"
#include "util/string.h"
namespace ToolkitUtil
{
class ShaderCompiler
{
public:

    /// constructor
    ShaderCompiler();
    /// destructor
    ~ShaderCompiler();

    /// set target platform
    void SetPlatform(Platform::Code platform);
    /// sets the source language
    void SetLanguage(const Util::String& lang);
    /// set source directory for base shaders
    void SetSrcShaderBaseDir(const Util::String& srcDir);
    /// set source directory for custom shaders
    void SetSrcShaderCustomDir(const Util::String& srcDir);
    /// set destination directory
    void SetDstShaderDir(const Util::String& dstDir);
    /// set frame shader source dir for base frame shaders
    void SetSrcFrameShaderBaseDir(const Util::String& srcDir);
    /// set frame shader source dir for custom frame shaders
    void SetSrcFrameShaderCustomDir(const Util::String& srcDir);
    /// set frame shader destination dir
    void SetDstFrameShaderDir(const Util::String& dstDir);
    /// set the materials source dir for base materials
    void SetSrcMaterialBaseDir(const Util::String& srcDir);
    /// set the materials source dir for custom materials
    void SetSrcMaterialCustomDir(const Util::String& srcDir);
    /// set the materials destination dir
    void SetDstMaterialsDir(const Util::String& dstDir);
    /// set force conversion flag (otherwise check timestamps)
    void SetForceFlag(bool b);
    /// set debugging flag
    void SetDebugFlag(bool b);
    /// set additional command line params
    void SetAdditionalParams(const Util::String& params);
    /// set quiet flag
    void SetQuietFlag(bool b);

    /// compile all shaders 
    bool CompileShaders();
    /// compile frame shader files
    bool CompileFrameShaders();
    /// compile materials
    bool CompileMaterials();

private:
    /// check whether a file needs a recompile (force flag and timestamps)
    bool CheckRecompile(const Util::String& srcPath, const Util::String& dstPath);
    /// compile shaders for HLSL
    bool CompileHLSL(const Util::String& srcPath);
    /// compiles shaders for glsl
    bool CompileGLSL(const Util::String& srcPath);
    /// compiles shaders for SPIRV
    bool CompileSPIRV(const Util::String& srcPath);
    /// write shader dictionary
    bool WriteShaderDictionary();

    Platform::Code platform;
    Util::String srcShaderBaseDir;
    Util::String srcShaderCustomDir;
    Util::String dstShaderDir;
    Util::String srcFrameShaderBaseDir;
    Util::String srcFrameShaderCustomDir;
    Util::String dstFrameShaderDir;
    Util::String srcMaterialBaseDir;
    Util::String srcMaterialCustomDir;
    Util::String dstMaterialDir;
    Util::String language;
    bool force;
    bool quiet;
    bool debug;
    Util::String additionalParams;
    Util::Array<Util::String> shaderNames;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderCompiler::SetPlatform(Platform::Code p)
{
    this->platform = p;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ShaderCompiler::SetLanguage( const Util::String& lang )
{
    this->language = lang;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderCompiler::SetSrcShaderBaseDir(const Util::String& d)
{
    this->srcShaderBaseDir = d;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ShaderCompiler::SetSrcShaderCustomDir( const Util::String& d )
{
    this->srcShaderCustomDir = d;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderCompiler::SetDstShaderDir(const Util::String& d)
{
    this->dstShaderDir = d;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderCompiler::SetSrcFrameShaderBaseDir(const Util::String& d)
{
    this->srcFrameShaderBaseDir = d;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ShaderCompiler::SetSrcFrameShaderCustomDir( const Util::String& d )
{
    this->srcFrameShaderCustomDir = d;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderCompiler::SetDstFrameShaderDir(const Util::String& d)
{
    this->dstFrameShaderDir = d;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ShaderCompiler::SetSrcMaterialBaseDir( const Util::String& d )
{
    this->srcMaterialBaseDir = d;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ShaderCompiler::SetSrcMaterialCustomDir( const Util::String& d )
{
    this->srcMaterialCustomDir = d;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ShaderCompiler::SetDstMaterialsDir( const Util::String& dstDir )
{
    this->dstMaterialDir = dstDir;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderCompiler::SetForceFlag(bool b)
{
    this->force = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderCompiler::SetDebugFlag(bool b)
{
    this->debug = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderCompiler::SetAdditionalParams(const Util::String& p)
{
    this->additionalParams = p;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderCompiler::SetQuietFlag(bool b)
{
    this->quiet = b;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------