//------------------------------------------------------------------------------
//  ogl4shaderinstance.cc
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4shaderinstance.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/shader.h"
#include "coregraphics/shadervariable.h"
#include "coregraphics/shadervariation.h"
#include "coregraphics/shaderserver.h"
#include "resources/resourcemanager.h"
#include "ogl4uniformbuffer.h"
#include "coregraphics/constantbuffer.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4ShaderInstance, 'D1SI', Base::ShaderInstanceBase);

using namespace Util;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
OGL4ShaderInstance::OGL4ShaderInstance() :
    inWireframe(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4ShaderInstance::~OGL4ShaderInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
    This method is called by Shader::CreateInstance() to setup the 
    new shader instance.
*/
void
OGL4ShaderInstance::Setup(const Ptr<CoreGraphics::Shader>& origShader)
{
    n_assert(origShader.isvalid());
    const Ptr<OGL4Shader>& ogl4Shader = origShader.upcast<OGL4Shader>();

    // call parent class
    ShaderInstanceBase::Setup(origShader);

	// copy effect pointer
	this->effect = ogl4Shader->GetOGL4Effect();

    int varblockCount = this->effect->GetNumVarblocks();
    for (int i = 0; i < varblockCount; i++)
    {
        // get varblock
        AnyFX::EffectVarblock* effectBlock = effect->GetVarblockByIndex(i);

        bool usedBySystem = false;
        if (effectBlock->HasAnnotation("System")) usedBySystem = effectBlock->GetAnnotationBool("System");

        // only create buffer if it's not using a reserved block
        if (!usedBySystem && effectBlock->IsActive())
        {
            // get bindings, and skip creating this buffer if the block is empty
            eastl::vector<AnyFX::VarblockVariableBinding> variableBinds = effectBlock->GetVariables();
            if (variableBinds.empty()) continue;

            // create a new buffer that will serve as this shaders backing storage
            Ptr<CoreGraphics::ConstantBuffer> uniformBuffer = CoreGraphics::ConstantBuffer::Create();

            // generate a name which we know will be unique
            Util::String name = effectBlock->GetName().c_str();
            n_assert(!this->uniformBuffersByName.Contains(name));

            // get variable corresponding to this block
            const Ptr<ShaderVariable>& blockVar = origShader->GetVariableByName(name);
            
            // setup block with a single buffer
            uniformBuffer->SetSize(effectBlock->GetSize());
            uniformBuffer->Setup(1);

			uniformBuffer->BeginUpdateSync();
            for (unsigned j = 0; j < variableBinds.size(); j++)
            {
                // find the shader variable and bind the constant buffer we just created to said variable
                const AnyFX::VarblockVariableBinding& binding = variableBinds[j];
                Util::String name = binding.name.c_str();
				uniformBuffer->UpdateArray(binding.value, binding.offset, binding.size, 1);
				this->uniformVariableBinds.Add(name, VariableBufferBinding(DeferredVariableToBufferBind{ binding.offset, binding.size, binding.arraySize }, uniformBuffer));
            }
			uniformBuffer->EndUpdateSync();

            // add to dictionaries
            this->uniformBuffers.Append(uniformBuffer);
            this->uniformBuffersByName.Add(name, uniformBuffer);
            this->blockToBufferBindings.Append(BlockBufferBinding(blockVar, uniformBuffer));
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderInstance::Reload(const Ptr<CoreGraphics::Shader>& origShader)
{
	n_assert(origShader.isvalid());
	const Ptr<OGL4Shader>& ogl4Shader = origShader.upcast<OGL4Shader>();

	// call parent class
	ShaderInstanceBase::Setup(origShader);

	// copy effect pointer
	this->effect = origShader->GetOGL4Effect();
	this->uniformVariableBinds.Clear();

	int varblockCount = this->effect->GetNumVarblocks();
	for (int i = 0; i < varblockCount; i++)
	{
		// get varblock
		AnyFX::EffectVarblock* effectBlock = effect->GetVarblockByIndex(i);

		bool usedBySystem = false;
		if (effectBlock->HasAnnotation("System")) usedBySystem = effectBlock->GetAnnotationBool("System");

		// only create buffer if it's not using a reserved block
		if (!usedBySystem && effectBlock->IsActive())
		{
			// get bindings, and skip creating this buffer if the block is empty
			eastl::vector<AnyFX::VarblockVariableBinding> variableBinds = effectBlock->GetVariables();
			if (variableBinds.empty()) continue;

			// generate a name which we know will be unique
			Util::String name = effectBlock->GetName().c_str();
			n_assert(!this->uniformBuffersByName.Contains(name));

			// get variable corresponding to this block
			const Ptr<ShaderVariable>& blockVar = origShader->GetVariableByName(name);

			// create a new buffer that will serve as this shaders backing storage
			Ptr<CoreGraphics::ConstantBuffer> uniformBuffer = this->uniformBuffersByName[name];

			// setup block again, with new size
			uniformBuffer->Discard();
			uniformBuffer->SetSize(effectBlock->GetSize());
			uniformBuffer->Setup(1);

			uniformBuffer->BeginUpdateSync();
			for (unsigned j = 0; j < variableBinds.size(); j++)
			{
				// find the shader variable and bind the constant buffer we just created to said variable
				const AnyFX::VarblockVariableBinding& binding = variableBinds[j];
				Util::String name = binding.name.c_str();
				uniformBuffer->UpdateArray(binding.value, binding.offset, binding.size, 1);
				this->uniformVariableBinds.Add(name, VariableBufferBinding(DeferredVariableToBufferBind{ binding.offset, binding.size, binding.arraySize }, uniformBuffer));
			}
			uniformBuffer->EndUpdateSync();
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderInstance::Cleanup()
{
    ShaderInstanceBase::Cleanup();
	this->effect = 0;

    IndexT i;
    for (i = 0; i < this->variableInstances.Size(); i++)
    {
        this->variableInstances[i]->Discard();
    }
    this->variableInstances.Clear();
    this->variableInstancesByName.Clear();
    this->uniformVariableBinds.Clear();

    for (i = 0; i < this->uniformBuffers.Size(); i++)
    {
        this->uniformBuffers[i]->Discard();
    }
    this->uniformBuffers.Clear();
    this->uniformBuffersByName.Clear();
	for (i = 0; i < this->blockToBufferBindings.Size(); i++)
	{
		this->blockToBufferBindings[i].Key()->SetBufferHandle(NULL);
	}
    this->blockToBufferBindings.Clear();
}

//------------------------------------------------------------------------------
/**
*/
bool
OGL4ShaderInstance::SelectActiveVariation(ShaderFeature::Mask featureMask)
{
    if (ShaderInstanceBase::SelectActiveVariation(featureMask))
        return true;
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderInstance::BeginUpdateSync()
{
	int i;
	for (i = 0; i < this->uniformBuffers.Size(); i++)
	{
		this->uniformBuffers[i]->BeginUpdateSync();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderInstance::EndUpdateSync()
{
	int i;
	for (i = 0; i < this->uniformBuffers.Size(); i++)
	{
		this->uniformBuffers[i]->EndUpdateSync();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderInstance::OnLostDevice()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderInstance::OnResetDevice()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderInstance::Commit()
{
    // apply the uniform buffers used by this shader instance
    IndexT i;
    for (i = 0; i < this->blockToBufferBindings.Size(); i++)
    {
        const BlockBufferBinding& binding = this->blockToBufferBindings[i];
        binding.Key()->SetBufferHandle(binding.Value()->GetHandle());
    }

	// run base class
    ShaderInstanceBase::Commit();

    for (i = 0; i < this->blockToBufferBindings.Size(); i++)
    {
        const BlockBufferBinding& binding = this->blockToBufferBindings[i];
        binding.Value()->CycleBuffers();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4ShaderInstance::SetWireframe(bool b)
{
    this->inWireframe = b;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<CoreGraphics::ShaderVariableInstance>
OGL4ShaderInstance::CreateVariableInstance(const Base::ShaderVariableBase::Name& n)
{
    Ptr<CoreGraphics::ShaderVariableInstance> var = ShaderInstanceBase::CreateVariableInstance(n);
    if (this->uniformVariableBinds.Contains(n))
    { 
        const DeferredVariableToBufferBind& binding = this->uniformVariableBinds[n].Key();
        const Ptr<CoreGraphics::ConstantBuffer>& buf = this->uniformVariableBinds[n].Value();
        var->BindToUniformBuffer(buf, binding.offset, binding.size);
    }
    
    return var;
}

} // namespace OpenGL4

