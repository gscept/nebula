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
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "coregraphics/shader.h"
#include "coregraphics/shaderfeature.h"
#include "coregraphics/shader.h"
#include "coregraphics/shaderidentifier.h"
#include "coregraphics/shaderpool.h"
#include "threading/safequeue.h"

namespace Threading
{
class Thread;
}

namespace IO
{
class FileWatcher;
class FileWatcherThread;
}

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

	/// explicitly loads a shader by resource id
	void LoadShader(const Resources::ResourceName& shdName);

	/// update shader server outside of frame
	void BeforeFrame();
	/// begin frame
	void BeforeView();
	/// end frame
	void AfterView();

protected:
    friend class CoreGraphics::ShaderIdentifier;
    friend class ShaderBase;

    CoreGraphics::ShaderIdentifier shaderIdentifierRegistry;
    CoreGraphics::ShaderFeature shaderFeature;
    CoreGraphics::ShaderFeature::Mask curShaderFeatureBits;
	Util::Dictionary<Resources::ResourceName, CoreGraphics::ShaderId> shaders;		
	Threading::SafeQueue<Resources::ResourceName> pendingShaderReloads;
	CoreGraphics::ShaderId sharedVariableShader;
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

} // namespace Base
//------------------------------------------------------------------------------

