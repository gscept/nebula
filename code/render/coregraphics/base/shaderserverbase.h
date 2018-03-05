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
#include "coregraphics/shader.h"
#include "coregraphics/shaderidentifier.h"
#include "coregraphics/shaderpool.h"

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
    bool HasShader(const Resources::ResourceName& resId) const;
	/// create a new shader state only for a set of groups
	CoreGraphics::ShaderStateId ShaderCreateState(const Resources::ResourceName& resId, const Util::Array<IndexT>& groups, bool createResourceSet = false);
	/// create a shared state, will only create a new shader state if one doesn't already exist with the given set of variable groups
	CoreGraphics::ShaderStateId ShaderCreateSharedState(const Resources::ResourceName& resId, const Util::Array<IndexT>& groups);
    /// get all loaded shaders
    const Util::Dictionary<Resources::ResourceName, CoreGraphics::ShaderId>& GetAllShaders() const;
	/// get shader by name
	const CoreGraphics::ShaderId GetShader(Resources::ResourceName resId) const;
	/// get name by shader id
	const Resources::ResourceName& GetName(const CoreGraphics::ShaderId& id) const;
    /// set currently active shader instance
    void SetActiveShader(const CoreGraphics::ShaderId shader);
    /// get currently active shader instance
    const CoreGraphics::ShaderId GetActiveShader() const;

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
    const CoreGraphics::ShaderConstantId GetSharedVariableByIndex(IndexT i) const;
	/// get the shared shader
	const CoreGraphics::ShaderId GetSharedShader();
    /// get the shared shader state
	const CoreGraphics::ShaderStateId GetSharedShaderState();

	/// reloads a shader
	void ReloadShader(const Resources::ResourceId shader);
	/// explicitly loads a shader by resource id
	void LoadShader(const Resources::ResourceName& shdName);

protected:
    friend class CoreGraphics::ShaderIdentifier;
    friend class ShaderBase;

    CoreGraphics::ShaderIdentifier shaderIdentifierRegistry;
    CoreGraphics::ShaderFeature shaderFeature;
    CoreGraphics::ShaderFeature::Mask curShaderFeatureBits;
	Util::Dictionary<Resources::ResourceName, CoreGraphics::ShaderId> shaders;
	Util::Dictionary<Util::StringAtom, CoreGraphics::ShaderStateId> sharedShaderStates;
	CoreGraphics::ShaderId sharedVariableShader;
	CoreGraphics::ShaderStateId sharedVariableShaderState;
    Ids::Id32 objectIdShaderVar;
	CoreGraphics::ShaderId activeShader;
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
ShaderServerBase::HasShader(const Resources::ResourceName& resId) const
{
    return this->shaders.Contains(resId);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Dictionary<Resources::ResourceName, CoreGraphics::ShaderId>&
ShaderServerBase::GetAllShaders() const
{
    return this->shaders;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::ShaderId
ShaderServerBase::GetShader(Resources::ResourceName resId) const
{
	return this->shaders[resId];
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName&
ShaderServerBase::GetName(const CoreGraphics::ShaderId& id) const
{
	return CoreGraphics::shaderPool->GetName(id);
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
ShaderServerBase::SetActiveShader(const CoreGraphics::ShaderId shader)
{
    this->activeShader = shader;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::ShaderId
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
    if (this->sharedVariableShaderState != Ids::InvalidId64)
		return CoreGraphics::ShaderGetConstantCount(this->sharedVariableShader);
    else
        return 0;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::ShaderConstantId
ShaderServerBase::GetSharedVariableByIndex(IndexT i) const
{
    n_assert(this->sharedVariableShaderState != CoreGraphics::ShaderStateId::Invalid());
	return CoreGraphics::ShaderStateGetConstant(this->sharedVariableShaderState, i);
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::ShaderId
ShaderServerBase::GetSharedShader()
{
	return this->sharedVariableShader;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::ShaderStateId
ShaderServerBase::GetSharedShaderState()
{
    return this->sharedVariableShaderState;
}

} // namespace Base
//------------------------------------------------------------------------------

