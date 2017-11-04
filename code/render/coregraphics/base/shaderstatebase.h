#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::ShaderStateBase
    
	A shader state is an object created from a shader, and contains an instance of its state.
	It doesn't contain the shader itself, but instead serves to contain uniforms,
	textures, and other types of buffer bindings, which can be committed when required.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/string.h"
#include "coregraphics/shadervariable.h"
#include "coregraphics/shaderfeature.h"
#include "coregraphics/shaderidentifier.h"
#include "coregraphics/shadervariableinstance.h"
#include "coregraphics/shader.h"
namespace CoreGraphics
{
class Shader;
class ShaderVariable;
}

//------------------------------------------------------------------------------
namespace Base
{
class ShaderStateBase : public Core::RefCounted
{
    __DeclareClass(ShaderStateBase);
public:
    /// constructor
    ShaderStateBase();
    /// destructor
    virtual ~ShaderStateBase();
    
    /// discard the shader instance, must be called when instance no longer needed
    void Discard();
    /// return true if this object is valid
    bool IsValid() const;
    /// get pointer to original shader which created this instance
    const Ptr<CoreGraphics::Shader>& GetShader() const;
    /// get shader code from original shader
    const CoreGraphics::ShaderIdentifier::Code& GetCode() const;

    /// create a new variable instance which will be applied when this shader instance gets applied
    Ptr<CoreGraphics::ShaderVariableInstance> CreateVariableInstance(const Base::ShaderVariableBase::Name& n);
    /// get variable instance based on name, asserts it exists
    const Ptr<CoreGraphics::ShaderVariableInstance>& GetVariableInstance(const Base::ShaderVariableBase::Name& n);
	/// discard variable instance
	void DiscardVariableInstance(const Ptr<CoreGraphics::ShaderVariableInstance>& var);

	/// return true if the shader instance has a variable by name
	bool HasVariableByName(const Base::ShaderVariableBase::Name& n) const;
	/// get number of variables
	SizeT GetNumVariables() const;
	/// get a variable by index
	const Ptr<CoreGraphics::ShaderVariable>& GetVariableByIndex(IndexT i) const;
	/// get a variable by name
	const Ptr<CoreGraphics::ShaderVariable>& GetVariableByName(const Base::ShaderVariableBase::Name& n) const;

    /// shortcut to select a shader variation through the original shader
    bool SelectActiveVariation(CoreGraphics::ShaderFeature::Mask mask);    

	/// begin all uniform buffers for a synchronous update
	void BeginUpdateSync();
	/// end buffer updates for all uniform buffers
	void EndUpdateSync();

    /// begin rendering through the currently selected variation, returns no. passes
    SizeT Begin();
    /// begin pass
    void BeginPass(IndexT passIndex);
    /// apply shader variables
    void Apply();
    /// commit changes before rendering
    void Commit();
    /// calls shading subsystem post draw callback
    void PostDraw();
	/// end pass
    void EndPass();
    /// end rendering through variation
    void End();

	/// set if shader state should be applied shared across all shaders
	void SetApplyShared(const bool b);
    
protected:   
    friend class ShaderBase;

    /// setup the shader instance from its original shader object
    virtual void Setup(const Ptr<CoreGraphics::Shader>& origShader);
	/// setup the shader instance from its original shader object
	virtual void Setup(const Ptr<CoreGraphics::Shader>& origShader, const Util::Array<IndexT>& groups, bool createResourceSet);
    /// discard the shader instance
    virtual void Cleanup();

    bool inBegin;
    bool inBeginPass;
	bool applyShared;
    Ptr<CoreGraphics::Shader> shader;
    Util::Array<Ptr<CoreGraphics::ShaderVariableInstance>> variableInstances;
    Util::Dictionary<Util::StringAtom, Ptr<CoreGraphics::ShaderVariableInstance>> variableInstancesByName;

	Util::Array<Ptr<CoreGraphics::ShaderVariable>> variables;
	Util::Dictionary<Base::ShaderVariableBase::Name, Ptr<CoreGraphics::ShaderVariable>> variablesByName;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::Shader>&
ShaderStateBase::GetShader() const
{
    return this->shader;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ShaderStateBase::HasVariableByName(const Base::ShaderVariableBase::Name& n) const
{
	return this->variablesByName.Contains(n);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
ShaderStateBase::GetNumVariables() const
{
	return this->variables.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderVariable>&
ShaderStateBase::GetVariableByIndex(IndexT i) const
{
	return this->variables[i];
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderVariable>&
ShaderStateBase::GetVariableByName(const CoreGraphics::ShaderVariable::Name& n) const
{
#if NEBULA3_DEBUG
	if (!this->HasVariableByName(n))
	{
		//n_error("Invalid shader variable name '%s' in shader '%s'",
			//n.Value(), CoreGraphics::shaderPool->GetResource().Value());
	}
#endif
	return this->variablesByName[n];
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderStateBase::SetApplyShared(const bool b)
{
	this->applyShared = b;
}

} // namespace CoreGraphics
//------------------------------------------------------------------------------


    