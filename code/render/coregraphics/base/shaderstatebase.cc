//------------------------------------------------------------------------------
//  shaderstatebase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/shader.h"
#include "coregraphics/base/shaderstatebase.h"
#include "coregraphics/shadervariation.h"
#include "coregraphics/shaderserver.h"

namespace Base
{
__ImplementClass(Base::ShaderStateBase, 'SSBS', Core::RefCounted);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
ShaderStateBase::ShaderStateBase() :
    inBegin(false),
    inBeginPass(false),
	applyShared(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ShaderStateBase::~ShaderStateBase()
{
    // check if Discard() has been called...
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
bool
ShaderStateBase::IsValid() const
{
    return this->shader.isvalid();
}

//------------------------------------------------------------------------------
/**
    This method must be called when the object is no longer needed
    for proper cleanup.
*/
void
ShaderStateBase::Discard()
{
    n_assert(this->IsValid());
    this->shader->DiscardShaderInstance((ShaderState*)this);
}

//------------------------------------------------------------------------------
/**
    Override this method in an API-specific subclass to setup the
    shader instance, and call the parent class for proper setup.
*/
void
ShaderStateBase::Setup(const Ptr<Shader>& origShader)
{
    n_assert(!this->IsValid());
    this->shader = origShader;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateBase::Setup(const Ptr<CoreGraphics::Shader>& origShader, const Util::Array<IndexT>& groups, bool createResourceSet)
{
	n_assert(!this->IsValid());
	this->shader = origShader;
	// override and handle group specific setup
}

//------------------------------------------------------------------------------
/**
    Override this method in an API-specific subclass to undo the
    setup in CreateInstance(), then call parent class to finalize
    the cleanup.
*/
void
ShaderStateBase::Cleanup()
{
    n_assert(this->IsValid());

	IndexT i;
	for (i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->Cleanup();
	}
	this->variables.Clear();
	this->variablesByName.Clear();
	this->shader = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ShaderStateBase::Begin()
{
    n_assert(!this->inBegin);
    n_assert(!this->inBeginPass);
    this->inBegin = true;
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateBase::BeginPass(IndexT passIndex)
{
    n_assert(this->inBegin);
    n_assert(!this->inBeginPass);
    this->inBeginPass = true;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateBase::Apply()
{
    IndexT i;
    for (i = 0; i < this->variableInstances.Size(); i++)
    {
        this->variableInstances[i]->Apply();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateBase::Commit()
{
    // also commit original shader
    this->shader->GetActiveVariation()->Commit();
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateBase::EndPass()
{
    n_assert(this->inBeginPass);
    this->inBeginPass = false;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateBase::End()
{
    n_assert(this->inBegin);
    n_assert(!this->inBeginPass);
    this->inBegin = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
ShaderStateBase::SelectActiveVariation(CoreGraphics::ShaderFeature::Mask mask)
{
    return this->shader->SelectActiveVariation(mask);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateBase::BeginUpdateSync()
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateBase::EndUpdateSync()
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderIdentifier::Code&
ShaderStateBase::GetCode() const
{
    return this->shader->GetCode();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<CoreGraphics::ShaderVariableInstance>
ShaderStateBase::CreateVariableInstance(const Base::ShaderVariableBase::Name& n)
{
    n_assert(!this->variableInstancesByName.Contains(n));
    Ptr<CoreGraphics::ShaderVariableInstance> instance = this->variablesByName[n]->CreateInstance();
    this->variableInstances.Append(instance);
    this->variableInstancesByName.Add(n, instance);
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<CoreGraphics::ShaderVariableInstance>&
ShaderStateBase::GetVariableInstance(const Base::ShaderVariableBase::Name& n)
{
    n_assert(this->variableInstancesByName.Contains(n));
    return this->variableInstancesByName[n];
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateBase::DiscardVariableInstance(const Ptr<CoreGraphics::ShaderVariableInstance>& var)
{
	IndexT i = this->variableInstances.FindIndex(var);
	n_assert(i != InvalidIndex);
	this->variableInstances.EraseIndex(i);
	this->variableInstancesByName.Erase(var->GetShaderVariable()->GetName());
	var->Discard();	
}

} // namespace Base