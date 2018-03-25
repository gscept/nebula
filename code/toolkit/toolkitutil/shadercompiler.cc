//------------------------------------------------------------------------------
//  shadercompiler.cc
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "shadercompiler.h"
#include "io/ioserver.h"
#include "io/xmlreader.h"
#include "coregraphics/config.h"

#if __DX11__
#include <d3dx11.h>
#include <D3Dcompiler.h>
#endif

#if __ANYFX__
#include "afxcompiler.h"
#endif
#include "converters/binaryxmlconverter.h"

using namespace Util;
using namespace IO;
namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
ShaderCompiler::ShaderCompiler() :
	language("HLSL"),
    srcShaderCustomDir(""),
    srcFrameShaderCustomDir(""),
    srcMaterialCustomDir(""),    
	platform(Platform::Win32),
	force(false),
	debug(false),
	quiet(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ShaderCompiler::~ShaderCompiler()
{
	// empty
}


//------------------------------------------------------------------------------
/**
*/
bool
ShaderCompiler::CheckRecompile(const Util::String& srcPath, const Util::String& dstPath)
{
	if (!this->force)
	{
		// check file stamps
		IoServer* ioServer = IoServer::Instance();
		if (ioServer->FileExists(dstPath))
		{
			FileTime srcTime = ioServer->GetFileWriteTime(srcPath);
			FileTime dstTime = ioServer->GetFileWriteTime(dstPath);
			if (dstTime > srcTime)
			{
				return false;
			}
		}
	}
	return true;
}
//------------------------------------------------------------------------------
/**
*/
bool 
ShaderCompiler::CompileShaders()
{
	n_assert(this->srcShaderBaseDir.IsValid());
	n_assert(this->dstShaderDir.IsValid());
	const Ptr<IoServer>& ioServer = IoServer::Instance();
	this->shaderNames.Clear();

	// check if source dir exists
	if (!ioServer->DirectoryExists(this->srcShaderBaseDir))
	{
		n_error("ERROR: shader source directory '%s' not found!\n", this->srcShaderBaseDir.AsCharPtr());
		return false;
	}

	// make sure the target directory exists
	ioServer->CreateDirectory(this->dstShaderDir);

	// attempt compile base shaders
	bool retval = false;
	if (this->language == "HLSL")
	{
		retval = this->CompileHLSL(this->srcShaderBaseDir);
	}
	else if (this->language == "GLSL")
	{
		retval = this->CompileGLSL(this->srcShaderBaseDir);
	}
	else if (this->language == "SPIRV")
	{
		retval = this->CompileSPIRV(this->srcShaderBaseDir);
	}

    if (this->srcFrameShaderCustomDir.IsValid())
    {
        // attempt to compile custom shaders
        if (this->language == "HLSL")
        {
            this->CompileHLSL(this->srcShaderCustomDir);
        }
        else if (this->language == "GLSL")
        {
            this->CompileGLSL(this->srcShaderCustomDir);
        }
		else if (this->language == "SPIRV")
		{
			this->CompileSPIRV(this->srcShaderCustomDir);
		}
    }    

	if (retval)
	{
		retval = this->WriteShaderDictionary();
	}

	return retval;
}

//------------------------------------------------------------------------------
/**
*/
bool 
ShaderCompiler::CompileFrameShaders()
{
	n_assert(this->srcFrameShaderBaseDir.IsValid());
	n_assert(this->dstFrameShaderDir.IsValid());
	const Ptr<IoServer>& ioServer = IoServer::Instance();

	// check if source dir exists
	if (!ioServer->DirectoryExists(this->srcFrameShaderBaseDir))
	{
		n_error("ERROR: frame shader source directory '%s' not found!\n", this->srcFrameShaderBaseDir.AsCharPtr());
		return false;
	}

	// make sure target dir exists
	ioServer->CreateDirectory(this->dstFrameShaderDir);

	// for each base frame shader...
	bool success = true;
	Array<String> srcFiles = ioServer->ListFiles(this->srcFrameShaderBaseDir, "*.json");
	IndexT i;
	for (i = 0; i < srcFiles.Size(); i++)
	{
		// build absolute source and target filenames
		String srcPath;
		srcPath.Format("%s/%s", this->srcFrameShaderBaseDir.AsCharPtr(), srcFiles[i].AsCharPtr());
		String dstPath;
		dstPath.Format("%s/%s", this->dstFrameShaderDir.AsCharPtr(), srcFiles[i].AsCharPtr());
		success &= ioServer->CopyFile(srcPath, dstPath);
		n_printf("Copied base frame script: %s ---> %s \n", srcPath.AsCharPtr(), dstPath.AsCharPtr());
	}

    if (this->srcFrameShaderCustomDir.IsValid())
    {
        // for each custom frame shader...
        srcFiles = ioServer->ListFiles(this->srcFrameShaderCustomDir, "*.json");
        for (i = 0; i < srcFiles.Size(); i++)
        {
            // build absolute source and target filenames
            String srcPath;
            srcPath.Format("%s/%s", this->srcFrameShaderCustomDir.AsCharPtr(), srcFiles[i].AsCharPtr());
            String dstPath;
            dstPath.Format("%s/%s", this->dstFrameShaderDir.AsCharPtr(), srcFiles[i].AsCharPtr());
			success &= ioServer->CopyFile(srcPath, dstPath);
            n_printf("Copied custom frame script: %s ---> %s \n", srcPath.AsCharPtr(), dstPath.AsCharPtr());
        }
    }
    
	return success;
}


//------------------------------------------------------------------------------
/**
*/
bool 
ShaderCompiler::CompileMaterials()
{
	n_assert(this->srcMaterialBaseDir.IsValid());
	n_assert(this->dstMaterialDir.IsValid());
	const Ptr<IoServer>& ioServer = IoServer::Instance();

	// check if source dir exists
	if (!ioServer->DirectoryExists(this->srcMaterialBaseDir))
	{
		n_warning("WARNING: materials source directory '%s' not found!\n", this->srcMaterialBaseDir.AsCharPtr());
		return false;
	}

    Util::String baseMaterialTemplateSrcDir = this->srcMaterialBaseDir;
    Util::String customMaterialTemplateSrcDir = this->srcMaterialCustomDir;
    Util::String materialTemplateDstDir = this->dstMaterialDir;

	// create converter
	BinaryXmlConverter converter;
	ToolkitUtil::Logger logger;
	converter.SetPlatform(Platform::Win32);

    // remove old directories
    ioServer->DeleteDirectory(materialTemplateDstDir);

	// make new
    ioServer->CreateDirectory(materialTemplateDstDir);

	// for each base material table...
	bool success = true;
    Array<String> srcFiles = ioServer->ListFiles(baseMaterialTemplateSrcDir, "*.xml");
	IndexT i;
	for (i = 0; i < srcFiles.Size(); i++)
	{
		// build absolute source and target filenames
		String srcPath;
        srcPath.Format("%s/%s", baseMaterialTemplateSrcDir.AsCharPtr(), srcFiles[i].AsCharPtr());
		String dstPath;
        dstPath.Format("%s/%s", materialTemplateDstDir.AsCharPtr(), srcFiles[i].AsCharPtr());

		// convert to binary xml
		success &= converter.ConvertFile(srcPath, dstPath, logger);
		n_printf("Copied base material template table: %s ---> %s \n", srcPath.AsCharPtr(), dstPath.AsCharPtr());
	}

    if (this->srcMaterialCustomDir.IsValid())
    {
        // for each custom material table...
        srcFiles = ioServer->ListFiles(customMaterialTemplateSrcDir, "*.xml");
        for (i = 0; i < srcFiles.Size(); i++)
        {
            // build absolute source and target filenames
            String srcPath;
            srcPath.Format("%s/%s", customMaterialTemplateSrcDir.AsCharPtr(), srcFiles[i].AsCharPtr());
            String dstPath;
            
            // format material to be extended with _custom
            String file = srcFiles[i];
            file.StripFileExtension();
            dstPath.Format("%s/%s_custom.xml", materialTemplateDstDir.AsCharPtr(), file.AsCharPtr());

            // copy file
			success &= converter.ConvertFile(srcPath, dstPath, logger);
            n_printf("Copied custom material table: %s ---> %s \n", srcPath.AsCharPtr(), dstPath.AsCharPtr());
        }
    }
    
	return success;
}


//------------------------------------------------------------------------------
/**
*/
bool 
ShaderCompiler::CompileHLSL(const Util::String& srcPath)
{
	const Ptr<IoServer>& ioServer = IoServer::Instance();

	// copy sdh files to destination
	bool retval = true;

	// list files in directory
	Array<String> srcFiles = ioServer->ListFiles(srcPath, "*.fx");

#ifndef __DX11__
	n_printf("Error: Cannot compile DX11 shaders without DX11 support\n");
	return false;
#endif

	// go through all files and compile
	IndexT j;
	for (j = 0; j < srcFiles.Size(); j++)
	{
		// get file
		String file = srcFiles[j];

		// compile
		n_printf("Compiling: %s\n", file.AsCharPtr());

		// format string with file
		String srcFile = srcPath + "/" + file;

		// format destination
		String destFile = this->dstShaderDir + "/" + file;
		destFile.StripFileExtension();
		file.StripFileExtension();

		// add to dictionary
		this->shaderNames.Append(file);

		if (this->CheckRecompile(srcFile, destFile))
		{
#if __DX11__
			ID3D10Blob* compiledShader = 0;
			ID3D10Blob* errorBlob = 0;

			HRESULT hr = D3DX11CompileFromFile(
				AssignRegistry::Instance()->ResolveAssignsInString(srcFile).AsCharPtr(),
				NULL,
				NULL,
				NULL,
				"fx_5_0",
				D3DCOMPILE_OPTIMIZATION_LEVEL3,
				NULL,
				NULL,
				&compiledShader,
				&errorBlob,
				&hr);

			if (FAILED(hr))
			{
				if (errorBlob)
				{
					String error;
					error.AppendRange((const char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
					n_printf("%s\n", error.AsCharPtr());
				}
				else
				{
					n_printf("Unhandled compilation error!\n");
				}
				retval = false;
			}
			else
			{


				// write compiled shader to file
				Ptr<Stream> stream = ioServer->CreateStream(destFile);
				stream->SetAccessMode(Stream::WriteAccess);

				// open stream
				if (stream->Open())
				{
					// write shader
					stream->Write(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize());

					// close file
					stream->Close();
				}
				else
				{
					n_printf("Couldn't open file %s\n", destFile.AsCharPtr());
					retval = false;
				}
			}
#endif
		}
		else
		{
			n_printf("No need to recompile %s\n", file.AsCharPtr());
			retval = true;
		}

	}
	return retval;
}

//------------------------------------------------------------------------------
/**
	Implemented using AnyFX
*/
bool 
ShaderCompiler::CompileGLSL(const Util::String& srcPath)
{
	const Ptr<IoServer>& ioServer = IoServer::Instance();

	// list files in directory
	Array<String> srcFiles = ioServer->ListFiles(srcPath, "*.fx");

#ifndef __ANYFX__
	n_printf("Error: Cannot compile DX11 shaders without DX11 support\n");
	return false;
#endif

#if __ANYFX__
	// start AnyFX compilation
	AnyFXBeginCompile();

	// go through all files and compile
	IndexT j;
	for (j = 0; j < srcFiles.Size(); j++)
	{
		// get file
		String file = srcFiles[j];

		// compile
		n_printf("Compiling: %s\n", file.AsCharPtr());

		// format string with file
		String srcFile = srcPath + "/" + file;

		// format destination
		String destFile = this->dstShaderDir + "/" + file;
		destFile.StripFileExtension();
		file.StripFileExtension();

		// add to dictionary
		this->shaderNames.Append(file);

		if (this->CheckRecompile(srcFile, destFile))
		{
			URI src(srcFile);
			URI dst(destFile);
            URI shaderFolder("toolkit:work/shaders/gl");
			std::vector<std::string> defines;
			std::vector<std::string> flags;
            Util::String define;
            define.Format("-D GLSL");
			defines.push_back(define.AsCharPtr());

            // first include this folder
            define.Format("-I%s/", URI(srcPath).LocalPath().AsCharPtr());
            defines.push_back(define.AsCharPtr());

            // then include the N3 toolkit shaders folder
            define.Format("-I%s/", shaderFolder.LocalPath().AsCharPtr());
            defines.push_back(define.AsCharPtr());

			// set flags
			flags.push_back("/NOSUB");		// deactivate subroutine usage
			flags.push_back("/GBLOCK");		// put all shader variables outside of explicit buffers in one global block

			// if using debug, output raw shader code
			if (this->debug)
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
			//escapedSrc.SubstituteString(" ", "\\ ");
			Util::String escapedDst = dst.LocalPath();
			//escapedDst.SubstituteString(" ", "\\ ");
			
			bool res = AnyFXCompile(escapedSrc.AsCharPtr(), escapedDst.AsCharPtr(), target.AsCharPtr(), "Khronos", defines, flags, &errors);
			if (!res)
			{
				if (errors)
				{
					n_error("%s\n", errors->buffer);
					delete errors;
					errors = 0;
				}
				return false;
			}
            else if (errors)
            {
                n_error("%s\n", errors->buffer);
                delete errors;
                errors = 0;
            }
		}
		else
		{
			n_printf("No need to recompile %s\n", file.AsCharPtr());
		}
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
ShaderCompiler::CompileSPIRV(const Util::String& srcPath)
{
	const Ptr<IoServer>& ioServer = IoServer::Instance();

	// list files in directory
	Array<String> srcFiles = ioServer->ListFiles(srcPath, "*.fx");

#ifndef __ANYFX__
	n_printf("Error: Cannot compile DX11 shaders without DX11 support\n");
	return false;
#endif

#if __ANYFX__
	// start AnyFX compilation
	AnyFXBeginCompile();

	// go through all files and compile
	IndexT j;
	for (j = 0; j < srcFiles.Size(); j++)
	{
		// get file
		String file = srcFiles[j];

		// compile
		n_printf("Compiling: %s\n", file.AsCharPtr());

		// format string with file
		String srcFile = srcPath + "/" + file;

		// format destination
		String destFile = this->dstShaderDir + "/" + file;
		destFile.StripFileExtension();
		file.StripFileExtension();

		// add to dictionary
		this->shaderNames.Append(file);

		URI shaderFolder(this->srcShaderBaseDir);
		std::vector<std::string> defines;
		std::vector<std::string> flags;
		Util::String define;
		define.Format("-D GLSL");
		defines.push_back(define.AsCharPtr());

		// first include this folder
		define.Format("-I%s/", URI(srcPath).LocalPath().AsCharPtr());
		defines.push_back(define.AsCharPtr());

		// then include the N3 toolkit shaders folder
		define.Format("-I%s/", shaderFolder.LocalPath().AsCharPtr());
		defines.push_back(define.AsCharPtr());

		URI src(srcFile);
		URI dst(destFile);

		bool needRecompile = false;

		// generate dependencies
		std::vector<std::string> deps = AnyFXGenerateDependencies(src.LocalPath().AsCharPtr(), defines);

		uint i;
		for (i = 1; i < deps.size(); i++)
		{
			Util::String dep = deps[i].c_str();
			needRecompile |= this->CheckRecompile(dep, destFile);
		}
		needRecompile |= this->CheckRecompile(srcFile, destFile);

		if (needRecompile)
		{
			// set flags
			flags.push_back("/NOSUB");			// deactivate subroutine usage, effectively expands all subroutines as functions
			flags.push_back("/GBLOCK");			// put all shader variables outside of an explicit block in one global block
			flags.push_back(Util::String::Sprintf("/DEFAULTSET %d", NEBULAT_DEFAULT_GROUP).AsCharPtr());	// since we want the most frequently switched set as high as possible, we send the default set to 8, must match the NEBULAT_DEFAULT_GROUP in std.fxh and DEFAULT_GROUP in coregraphics/config.h

			// if using debug, output raw shader code
			if (this->debug)
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
			//escapedSrc.SubstituteString(" ", "\\ ");
			Util::String escapedDst = dst.LocalPath();
			//escapedDst.SubstituteString(" ", "\\ ");

			bool res = AnyFXCompile(escapedSrc.AsCharPtr(), escapedDst.AsCharPtr(), target.AsCharPtr(), "Khronos", defines, flags, &errors);
			if (!res)
			{
				if (errors)
				{
					n_error("%s\n", errors->buffer);
					delete errors;
					errors = 0;
				}
				return false;
			}
			else if (errors)
			{
				n_error("%s\n", errors->buffer);
				delete errors;
				errors = 0;
			}
		}
		else
		{
			n_printf("No need to recompile %s\n", file.AsCharPtr());
		}
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
ShaderCompiler::WriteShaderDictionary()
{
	String filename;
	filename.Format("%s/shaders.dic", this->dstShaderDir.AsCharPtr());
	Ptr<Stream> stream = IoServer::Instance()->CreateStream(filename);
	Ptr<TextWriter> textWriter = TextWriter::Create();
	textWriter->SetStream(stream);
	if (textWriter->Open())
	{
		String prefix = "shd:";
		IndexT i;
		for (i = 0; i < this->shaderNames.Size(); i++)
		{
			textWriter->WriteLine(prefix + this->shaderNames[i]);        
		}
		textWriter->Close();
		return true;
	}
	return false;
}


} // namespace ToolkitUtil