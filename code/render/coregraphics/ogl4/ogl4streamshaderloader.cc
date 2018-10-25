//------------------------------------------------------------------------------
//  ogl4streamshaderloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "effectfactory.h"
#include "coregraphics/ogl4/ogl4streamshaderloader.h"
#include "coregraphics/ogl4/ogl4shader.h"
#include "coregraphics/ogl4/ogl4renderdevice.h"
#include "coregraphics/ogl4/ogl4shaderserver.h"
#include "io/ioserver.h"
#include "../shadervariation.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4StreamShaderLoader, 'D1SL', Resources::StreamResourceLoader);

using namespace Resources;
using namespace CoreGraphics;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
bool
OGL4StreamShaderLoader::CanLoadAsync() const
{
    // no asynchronous loading supported for shader
    return false;
}

//------------------------------------------------------------------------------
/**
    Loads a uncompiled shader files from a stream into 2-5 shader programs which are then compiled and linked
*/
bool
OGL4StreamShaderLoader::SetupResourceFromStream(const Ptr<Stream>& stream)
{
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());
    n_assert(this->resource->IsA(OGL4Shader::RTTI));
    const Ptr<OGL4Shader>& res = this->resource.downcast<OGL4Shader>();
    n_assert(!res->IsLoaded());
    
    // map stream to memory
    stream->SetAccessMode(Stream::ReadAccess);
    if (stream->Open())
    {		
        void* srcData = stream->Map();
        uint srcDataSize = stream->GetSize();

		// load effect from memory
		AnyFX::Effect* effect = AnyFX::EffectFactory::Instance()->CreateEffectFromMemory(srcData, srcDataSize);

		// catch any potential GL error coming from AnyFX
		n_assert(GLSUCCESS);
		if (!effect)
		{
			n_error("OGL4StreamShaderLoader: failed to load shader '%s'!", 
				res->GetResourceId().Value());
			return false;
		}
		
		res->ogl4Effect = effect;
		res->shaderName = res->GetResourceId().AsString();
        res->shaderIdentifierCode = ShaderIdentifier::FromName(res->shaderName.AsString());

        // setup shader variations
        int programCount = effect->GetNumPrograms();
        for (int i = 0; i < programCount; i++)
        {
            // a shader variation in Nebula is equal to a program object in AnyFX
            Ptr<ShaderVariation> variation = ShaderVariation::Create();

            // get program object from shader subsystem
            AnyFX::EffectProgram* program = effect->GetProgramByIndex(i);

            if (program->IsValid())
            {
                variation->Setup(program);
                res->variations.Add(variation->GetFeatureMask(), variation);
            }
        }

        // make sure that the shader has one variation selected
        if (!res->variations.IsEmpty()) res->activeVariation = res->variations.ValueAtIndex(0);

        // load uniforms
        int variableCount = effect->GetNumVariables();
        for (int i = 0; i < variableCount; i++)
        {
            // get AnyFX variable
            AnyFX::EffectVariable* effectVar = effect->GetVariableByIndex(i);
            if (effectVar->IsActive())
            {
				// create new variable
				Ptr<ShaderVariable> var = ShaderVariable::Create();

                // setup variable from AnyFX variable
                var->Setup(effectVar);
                res->variables.Append(var);
                res->variablesByName.Add(var->GetName(), var);
            }
        }

        // load shader storage buffer variables
        int varbufferCount = effect->GetNumVarbuffers();
        for (int i = 0; i < varbufferCount; i++)
        {
            // get AnyFX variable
            AnyFX::EffectVarbuffer* effectBuf = effect->GetVarbufferByIndex(i);
			if (effectBuf->IsActive())
			{
				// create new variable
				Ptr<ShaderVariable> var = ShaderVariable::Create();

				var->Setup(effectBuf);
				res->variables.Append(var);
				res->variablesByName.Add(var->GetName(), var);
			}
        }

        // load uniform block variables
        int varblockCount = effect->GetNumVarblocks();
        for (int i = 0; i < varblockCount; i++)
        {
            // get varblock
            AnyFX::EffectVarblock* effectBlock = effect->GetVarblockByIndex(i);
			if (effectBlock->IsActive())
			{
				// create a new variable
				Ptr<ShaderVariable> var = ShaderVariable::Create();

				if (effectBlock->IsActive())
				{
					// setup variable on varblock, this allow us to set the buffer the block should use
					var->Setup(effectBlock);
					res->variables.Append(var);
					res->variablesByName.Add(var->GetName(), var);
				}
			}
        }

        // setup variables belonging to the 'default' variable block, which is where all global variables end up
        if (effect->HasVarblock("GlobalBlock"))
        {
            // get global block
            AnyFX::EffectVarblock* effectBlock = effect->GetVarblockByName("GlobalBlock");

            eastl::vector<AnyFX::VarblockVariableBinding> variableBinds = effectBlock->GetVariables();
            if (!variableBinds.empty() && effectBlock->IsActive())
            {
                // create constant buffer, make it sync because we might have > 1 updates per frame
                res->globalBlockBuffer = ConstantBuffer::Create();
                res->globalBlockBuffer->SetSize(effectBlock->GetSize());
                res->globalBlockBuffer->SetSync(true);
                res->globalBlockBuffer->Setup(2);

				res->globalBlockBuffer->BeginUpdateSync();
                for (unsigned j = 0; j < variableBinds.size(); j++)
                {
                    // bind this variable to a shader global buffer, if this variable is instanced, it can either point to its
                    // own buffer, or point directly to this block (if variable instance is not bound to a buffer)
                    const AnyFX::VarblockVariableBinding& binding = variableBinds[j];
                    const Ptr<ShaderVariable>& var = res->variablesByName[binding.name.c_str()];
                    var->BindToUniformBuffer(res->globalBlockBuffer, binding.offset, binding.size, binding.value);
					delete[] binding.value;	// delete value created as a copy from AnyFX
                }
				res->globalBlockBuffer->EndUpdateSync();
				res->globalBlockBufferVar = res->variablesByName["GlobalBlock"];
            }
        }

		return true;
	}
    return false;
}


} // namespace OpenGL4

