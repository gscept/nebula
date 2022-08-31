//------------------------------------------------------------------------------
//  shaderc.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "singleshadercompiler.h"
#include "io/ioserver.h"
#include "coregraphics/config.h"
#include "app/application.h"
#include "toolkit-common/converters/binaryxmlconverter.h"

#if __ANYFX__
#include "afxcompiler.h"
#endif

using namespace Util;
using namespace IO;
namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
SingleShaderCompiler::SingleShaderCompiler() :
    language("SPIRV"),          
    platform(Platform::Win32),  
    debug(false),
    quiet(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SingleShaderCompiler::~SingleShaderCompiler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
SingleShaderCompiler::CompileShader(const Util::String& src)
{
    if (!this->dstDir.IsValid())
    {
        n_printf("shaderc error: No destination for shader compile");
        return false;
    }

    if (!this->headerDir.IsValid())
    {
        n_printf("shaderc error: No header output folder for shader compile");
        return false;
    }
    
    const Ptr<IoServer>& ioServer = IoServer::Instance();
    
    // check if source 
    if (!ioServer->FileExists(src))
    {
        n_printf("[shaderc] error: shader source '%s' not found!\n", src.AsCharPtr());
        return false;
    }

    // make sure the target directory exists
    ioServer->CreateDirectory(this->dstDir + "/shaders");
    ioServer->CreateDirectory(this->headerDir);

    // attempt compile base shaders
    bool retval = false;
    if (this->language == "GLSL")
    {
        retval = this->CompileGLSL(src);
    }
    else if (this->language == "SPIRV")
    {
        retval = this->CompileSPIRV(src);
    }
    
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
bool
SingleShaderCompiler::CompileFrameShader(const Util::String& srcf)
{
    const Ptr<IoServer>& ioServer = IoServer::Instance();

    // check if source dir exists
    if (!ioServer->FileExists(URI(srcf)))
    {
        n_printf("[shaderc] error: frame shader source  '%s' not found!\n", srcf.AsCharPtr());
        return false;
    }

    // make sure target dir exists
    Util::String frameDest = this->dstDir + "/frame/";
    ioServer->CreateDirectory(frameDest);
    frameDest.Append(srcf.ExtractFileName());
    ioServer->CopyFile(srcf, frameDest);
    n_printf("[shaderc] Converted base frame script:\n   %s ---> %s \n", srcf.AsCharPtr(), frameDest.AsCharPtr());
    
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool 
SingleShaderCompiler::CompileMaterial(const Util::String & srcf)
{   
    const Ptr<IoServer>& ioServer = IoServer::Instance();

    // create converter
    BinaryXmlConverter converter;
    ToolkitUtil::Logger logger;
    logger.SetVerbose(false);
    converter.SetPlatform(Platform::Win32);
    
    // make sure output exists
    Util::String destDir = this->dstDir + "/materials";
    ioServer->CreateDirectory(destDir);
    
    Util::String dest = destDir + "/" + srcf.ExtractFileName();
    
    URI src(srcf);
    URI dst(dest);
    Logger dummy;
    // convert to binary xml
    n_printf("[shaderc] Converting base material template table:\n   %s ---> %s \n", src.LocalPath().AsCharPtr(), dst.LocalPath().AsCharPtr());
    return converter.ConvertFile(srcf, dest, dummy);
}

//------------------------------------------------------------------------------
/**
    Implemented using AnyFX
*/
bool 
SingleShaderCompiler::CompileGLSL(const Util::String& srcf)
{
    const Ptr<IoServer>& ioServer = IoServer::Instance();

#ifndef __ANYFX__
    n_printf("Error: Cannot compile DX11 shaders without DX11 support\n");
    return false;
#endif

#if __ANYFX__
    // start AnyFX compilation
    AnyFXBeginCompile();

    Util::String file = srcf.ExtractFileName();
    file.StripFileExtension();
    // format destination
    String destFile = this->dstDir + "/shaders/" + file;

    URI src(srcf);
    URI dst(destFile);

    // compile
    n_printf("[shaderc] Compiling:\n   %s -> %s\n", src.LocalPath().AsCharPtr(), dst.LocalPath().AsCharPtr());

    std::vector<std::string> defines;
    std::vector<std::string> flags;
    Util::String define;
    define.Format("-D GLSL");
    defines.push_back(define.AsCharPtr());

    // first include this folder
    define.Format("-I%s/", URI(srcf).LocalPath().AsCharPtr());
    defines.push_back(define.AsCharPtr());

    for (auto inc = this->includeDirs.Begin(); inc != this->includeDirs.End(); inc++)
    {
        define.Format("-I%s/", URI(*inc).LocalPath().AsCharPtr());
        defines.push_back(define.AsCharPtr());
    }

    // set flags
    flags.push_back("/NOSUB");      // deactivate subroutine usage
    flags.push_back("/GBLOCK");     // put all shader variables outside of explicit buffers in one global block

    // if using debug, output raw shader code
    if (!this->debug)
    {
        flags.push_back("/O");
    }

    AnyFXErrorBlob* errors = NULL;

    // this will get the highest possible value for the GL version, now clamp the minor and major to the one supported by glew
    int major = 4;
    int minor = 4;

    Util::String target;
    target.Format("gl%d%d", major, minor);
    Util::String escapedSrc = src.LocalPath();
    Util::String escapedDst = dst.LocalPath();
    std::vector<std::pair<unsigned, std::string>> resourceTableNames = {
        std::make_pair(NEBULA_TICK_GROUP, "Tick")
        , std::make_pair(NEBULA_FRAME_GROUP, "Frame")
        , std::make_pair(NEBULA_PASS_GROUP, "Pass")
        , std::make_pair(NEBULA_BATCH_GROUP, "Batch")
        , std::make_pair(NEBULA_INSTANCE_GROUP, "Instance")
        , std::make_pair(NEBULA_SYSTEM_GROUP, "System")
        , std::make_pair(NEBULA_DYNAMIC_OFFSET_GROUP, "DynamicOffset")
    };

    bool res = AnyFXCompile(
        escapedSrc.AsCharPtr()
        , escapedDst.AsCharPtr()
        , target.AsCharPtr()
        , nullptr
        , "Khronos"
        , defines
        , flags
        , resourceTableNames
        , &errors
    );

    if (!res)
    {
        if (errors)
        {
            n_printf("%s\n", errors->buffer);
            delete errors;
            errors = 0;
        }
        return false;
    }
    else if (errors)
    {
        n_printf("%s\n", errors->buffer);
        delete errors;
        errors = 0;
    }

    // stop AnyFX compilation
    AnyFXEndCompile();
#else
#error "No GLSL compiler implemented! (use definition __ANYFX__ to fix this)"
#endif

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
SingleShaderCompiler::CompileSPIRV(const Util::String& srcf)
{
    const Ptr<IoServer>& ioServer = IoServer::Instance();

#ifndef __ANYFX__
    n_printf("Error: Cannot compile DX11 shaders without DX11 support\n");
    return false;
#endif

#if __ANYFX__
    // start AnyFX compilation
    AnyFXBeginCompile();

    
    Util::String file = srcf.ExtractFileName();
    Util::String folder = srcf.ExtractDirName();
    file.StripFileExtension();

    // format destination
    String destFile = this->dstDir + "/shaders/" + file + ".fxb";
    String destHeader = this->headerDir + "/" + file + ".h";

    URI src(srcf);
    URI dst(destFile);
    URI dstH(destHeader);

    // compile
    n_printf("[shaderc] \n Compiling:\n   %s -> %s", src.LocalPath().AsCharPtr(), dst.LocalPath().AsCharPtr());
    n_printf("          \n Generating:\n   %s -> %s\n", src.LocalPath().AsCharPtr(), dstH.LocalPath().AsCharPtr());

    
    std::vector<std::string> defines;
    std::vector<std::string> flags;
    Util::String define;
    define.Format("-D GLSL");
    defines.push_back(define.AsCharPtr());

    // first include this folder
    define.Format("-I%s/", URI(folder).LocalPath().AsCharPtr());
    defines.push_back(define.AsCharPtr());

    for (auto inc = this->includeDirs.Begin(); inc != this->includeDirs.End(); inc++)
    {
        define.Format("-I%s/", URI(*inc).LocalPath().AsCharPtr());
        defines.push_back(define.AsCharPtr());
    }

    // set flags
    flags.push_back("/NOSUB");          // deactivate subroutine usage, effectively expands all subroutines as functions
    flags.push_back("/GBLOCK");         // put all shader variables outside of an explicit block in one global block
    flags.push_back(Util::String::Sprintf("/DEFAULTSET %d", NEBULA_BATCH_GROUP).AsCharPtr());   // since we want the most frequently switched set as high as possible, we send the default set to 8, must match the NEBULAT_DEFAULT_GROUP in std.fxh and DEFAULT_GROUP in coregraphics/config.h

    // if using debug, output raw shader code
    if (!this->debug)
    {
        flags.push_back("/O");
    }

    AnyFXErrorBlob* errors = NULL;

    // this will get the highest possible value for the GL version, now clamp the minor and major to the one supported by glew
    int major = 1;
    int minor = 0;

    Util::String target;
    target.Format("spv%d%d", major, minor);
    Util::String escapedSrc = src.LocalPath();
    Util::String escapedDst = dst.LocalPath();
    Util::String escapedHeader = dstH.LocalPath();
    std::vector<std::pair<unsigned, std::string>> resourceTableNames = {
        std::make_pair(NEBULA_TICK_GROUP, "Tick")
        , std::make_pair(NEBULA_FRAME_GROUP, "Frame")
        , std::make_pair(NEBULA_PASS_GROUP, "Pass")
        , std::make_pair(NEBULA_BATCH_GROUP, "Batch")
        , std::make_pair(NEBULA_INSTANCE_GROUP, "Instance")
        , std::make_pair(NEBULA_SYSTEM_GROUP, "System")
        , std::make_pair(NEBULA_DYNAMIC_OFFSET_GROUP, "DynamicOffset")
    };

    bool res = AnyFXCompile(
        escapedSrc.AsCharPtr()
        , escapedDst.AsCharPtr()
        , escapedHeader.AsCharPtr()
        , target.AsCharPtr()
        , "Khronos"
        , defines
        , flags
        , resourceTableNames
        , &errors
    );
    if (!res)
    {
        if (errors)
        {
            n_printf("%s\n", errors->buffer);
            delete errors;
            errors = 0;
        }
        return false;
    }
    else if (errors)
    {
        n_printf("%s\n", errors->buffer);
        delete errors;
        errors = 0;
    }
    // stop AnyFX compilation
    AnyFXEndCompile();
#else
#error "No SPIR-V compiler implemented! (use definition __ANYFX__ to fix this)"
#endif

    return true;
}


//------------------------------------------------------------------------------
/**
*/
bool
SingleShaderCompiler::CreateDependencies(const Util::String& srcf)
{
const Ptr<IoServer>& ioServer = IoServer::Instance();

#ifndef __ANYFX__
    n_printf("Error: Cannot compile DX11 shaders without DX11 support\n");
    return false;
#endif

    // start AnyFX compilation
    //AnyFXBeginCompile();

    
    Util::String file = srcf.ExtractFileName();
    Util::String folder = srcf.ExtractDirName();
    file.StripFileExtension();
    // format destination
    String destDir = this->dstDir + "/shaders/";
    String destFile = destDir + file + ".dep";

    URI src(srcf);
    URI dst(destFile);


    // compile
    n_printf("[shaderc] Analyzing:\n   %s\n", src.LocalPath().AsCharPtr());
    
    std::vector<std::string> defines;
    
    Util::String define;
    define.Format("-D GLSL");
    defines.push_back(define.AsCharPtr());

    // first include this folder
    define.Format("-I%s/", URI(folder).LocalPath().AsCharPtr());
    defines.push_back(define.AsCharPtr());

    for (auto inc = this->includeDirs.Begin(); inc != this->includeDirs.End(); inc++)
    {
        define.Format("-I%s/", URI(*inc).LocalPath().AsCharPtr());
        defines.push_back(define.AsCharPtr());
    }
    
    Util::String escapedSrc = src.LocalPath();
    
    ioServer->CreateDirectory(destDir);
    Ptr<IO::Stream> output = ioServer->CreateStream(destFile);
    Ptr<IO::TextWriter> writer = IO::TextWriter::Create();
    writer->SetStream(output);
    if(writer->Open())
    {
        std::vector<std::string> deps = AnyFXGenerateDependencies(escapedSrc.AsCharPtr(), defines);
        for(auto str : deps)
        {                        
            writer->WriteString(str.c_str());
            writer->WriteChar(';');
        }
        writer->Close();
    }
    
    return true;
}


} // namespace ToolkitUtil