#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::ShaderServerBase

	The ShaderServer loads all shaders when created, meaning all shaders
	in the project must be valid and hardware compatible when starting.

	A shader only contains an applicable program, but to apply uniform values
	such as textures and transforms a ShaderState is required.

	ShaderStates can be created directly from the shader, however it is recommended
	to through the shader server. Creating a shader state means that the shader
	state contains its own setup of values, which when committed will be actually
	applied within the shader program.

	The shader server can also create shared states, which just returns a unique
	state containing the variable groups, and will modify the same shader state if
	any variable is applied to it. 
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "coregraphics/shader.h"
#include "coregraphics/shaderfeature.h"
#include "coregraphics/shadervariable.h"
#include "coregraphics/shaderstate.h"
#include "coregraphics/shaderidentifier.h"

namespace CoreGraphics
{
    class Shader;
}

//------------------------------------------------------------------------------
namespace Base
{
class ShaderServerBase : public Core::RefCounted
{
    __DeclareClass(ShaderServerBase);
    __DeclareSingleton(ShaderServerBase);
public:
    /// constructor
    ShaderServerBase();
    /// destructor
    virtual ~ShaderServerBase();
    
    /// open the shader server
    bool Open();
    /// close the shader server
    void Close();
    /// return true if the shader server is open
    bool IsOpen() const;

    /// return true if a shader exists
    bool HasShader(const Resources::ResourceId& resId) const;
	/// create a new shader state only for a set of groups
	Ptr<CoreGraphics::ShaderState> CreateShaderState(const Resources::ResourceId& resId, const Util::Array<IndexT>& groups, bool createResourceSet = false);
	/// create a shared state, will only create a new shader state if one doesn't already exist with the given set of variable groups
	Ptr<CoreGraphics::ShaderState> CreateSharedShaderState(const Resources::ResourceId& resId, const Util::Array<IndexT>& groups);
    /// get all loaded shaders
    const Util::Dictionary<Resources::ResourceId, Ptr<CoreGraphics::Shader> >& GetAllShaders() const;
	/// get shader by name
	const Ptr<CoreGraphics::Shader>& GetShader(Resources::ResourceId resId) const;
    /// set currently active shader instance
    void SetActiveShader(const Ptr<CoreGraphics::Shader>& shader);
    /// get currently active shader instance
    const Ptr<CoreGraphics::Shader>& GetActiveShader() const;

    /// reset the current feature bits
    void ResetFeatureBits();
    /// set shader feature by bit mask
    void SetFeatureBits(CoreGraphics::ShaderFeature::Mask m);
    /// clear shader feature by bit mask
    void ClearFeatureBits(CoreGraphics::ShaderFeature::Mask m);
    /// get the current feature mask
    CoreGraphics::ShaderFeature::Mask GetFeatureBits() const;
    /// convert a shader feature string into a feature bit mask
    CoreGraphics::ShaderFeature::Mask FeatureStringToMask(const Util::String& str);
    /// convert shader feature bit mask into string
    Util::String FeatureMaskToString(CoreGraphics::ShaderFeature::Mask mask);

    /// apply an object id
    void ApplyObjectId(IndexT i);

    /// get number of shared variables
    SizeT GetNumSharedVariables() const;
    /// get a shared variable by index
    const Ptr<CoreGraphics::ShaderVariable>& GetSharedVariableByIndex(IndexT i) const;
    /// get the shared shader
	const Ptr<CoreGraphics::ShaderState>& GetSharedShader();

	/// reloads a shader
	void ReloadShader(Ptr<CoreGraphics::Shader> shader);
	/// explicitly loads a shader by resource id
	void LoadShader(const Resources::ResourceId& shdName);

protected:
    friend class CoreGraphics::ShaderIdentifier;
    friend class ShaderBase;

    CoreGraphics::ShaderIdentifier shaderIdentifierRegistry;
    CoreGraphics::ShaderFeature shaderFeature;
    CoreGraphics::ShaderFeature::Mask curShaderFeatureBits;
	Util::Dictionary<Resources::ResourceId, Ptr<CoreGraphics::Shader>> shaders;
	Util::Dictionary<Util::StringAtom, Ptr<CoreGraphics::ShaderState>> sharedShaderStates;
	Ptr<CoreGraphics::ShaderState> sharedVariableShader;
    Ptr<CoreGraphics::ShaderVariable> objectIdShaderVar;
    Ptr<CoreGraphics::Shader> activeShader;
    bool isOpen;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
ShaderServerBase::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ShaderServerBase::HasShader(const Resources::ResourceId& resId) const
{
    return this->shaders.Contains(resId);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Dictionary<Resources::ResourceId, Ptr<CoreGraphics::Shader> >&
ShaderServerBase::GetAllShaders() const
{
    return this->shaders;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::Shader>&
ShaderServerBase::GetShader(Resources::ResourceId resId) const
{
	return this->shaders[resId];
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::ShaderFeature::Mask
ShaderServerBase::FeatureStringToMask(const Util::String& str)
{
    return this->shaderFeature.StringToMask(str);
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
ShaderServerBase::FeatureMaskToString(CoreGraphics::ShaderFeature::Mask mask)
{
    return this->shaderFeature.MaskToString(mask);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderServerBase::ResetFeatureBits()
{
    this->curShaderFeatureBits = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderServerBase::SetFeatureBits(CoreGraphics::ShaderFeature::Mask m)
{
    this->curShaderFeatureBits |= m;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderServerBase::ClearFeatureBits(CoreGraphics::ShaderFeature::Mask m)
{
    this->curShaderFeatureBits &= ~m;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::ShaderFeature::Mask
ShaderServerBase::GetFeatureBits() const
{
    return this->curShaderFeatureBits;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderServerBase::SetActiveShader(const Ptr<CoreGraphics::Shader>& shader)
{
    this->activeShader = shader;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::Shader>&
ShaderServerBase::GetActiveShader() const
{
    return this->activeShader;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
ShaderServerBase::GetNumSharedVariables() const
{
    if (this->sharedVariableShader.isvalid())
    {        
        return this->sharedVariableShader->GetNumVariables();
    }
    else
    {
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderVariable>&
ShaderServerBase::GetSharedVariableByIndex(IndexT i) const
{
    n_assert(this->sharedVariableShader.isvalid());
    return this->sharedVariableShader->GetVariableByIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderState>&
ShaderServerBase::GetSharedShader()
{
    return this->sharedVariableShader;
}

} // namespace Base
//------------------------------------------------------------------------------

