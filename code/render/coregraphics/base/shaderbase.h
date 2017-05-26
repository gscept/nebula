#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::ShaderBase
  
    A shader object groups render states required to render a piece of
    geometry. Shader objects can be used as a shader object, which can apply
    a complete shader pipeline.

    A shader also contains a list of variables, which can be set, thus changing
    the global state of the shader.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "resources/resource.h"
#include "vertexcomponentbase.h"
#include "coregraphics/shaderidentifier.h"
#include "coregraphics/shaderfeature.h"
#include "coregraphics/shadervariable.h"
#include "coregraphics/shadervariation.h"
#include "core/config.h"

namespace CoreGraphics
{
class ShaderVariation;
class ShaderState;
};

//------------------------------------------------------------------------------
namespace Base
{
class ShaderBase : public Resources::Resource
{
    __DeclareClass(ShaderBase);
public:
    /// constructor
    ShaderBase();
    /// destructor
    virtual ~ShaderBase();

    /// unload shader
    void Unload();
	/// create a shader instance from this shader implementing a subset of shader variables described by the array of sets
	Ptr<CoreGraphics::ShaderState> CreateState(const Util::Array<IndexT>& groups, bool createResourceSet = false);
    /// discard a shader instance
    void DiscardShaderInstance(const Ptr<CoreGraphics::ShaderState>& inst);
    /// get all instances
    const Util::Array<Ptr<CoreGraphics::ShaderState>>& GetAllShaderStates() const;
	/// get shader name
	const Util::StringAtom GetShaderName() const;
    /// get unique shader identifier code
    const CoreGraphics::ShaderIdentifier::Code& GetCode() const;
	/// get the state associated with the creation of the shader
	const Ptr<CoreGraphics::ShaderState>& GetMainState();

#if __NEBULA3_HTTP__
	/// get the debug shader state
	const Ptr<CoreGraphics::ShaderState>& GetDebugState();
#endif

    /// return true if variation exists by matching feature mask
    bool HasVariation(CoreGraphics::ShaderFeature::Mask featureMask) const;
    /// get number of variations in the shader
    SizeT GetNumVariations() const;
    /// get shader variation by index
    const Ptr<CoreGraphics::ShaderVariation>& GetVariationByIndex(IndexT i) const;
    /// get shader variation by feature mask
    const Ptr<CoreGraphics::ShaderVariation>& GetVariationByFeatureMask(CoreGraphics::ShaderFeature::Mask featureMask) const;
    /// select active variation by feature mask, return true if active variation has been changed
    bool SelectActiveVariation(CoreGraphics::ShaderFeature::Mask featureMask);
    /// get the currently active variation
    const Ptr<CoreGraphics::ShaderVariation>& GetActiveVariation() const;

    /// begin updating shader state
    void BeginUpdate();
    /// end updating shader state
    void EndUpdate();
    /// apply currently selected variation
    void Apply();
    /// commit pending changes to the shader (variables)
    void Commit();

protected:
    CoreGraphics::ShaderIdentifier::Code shaderIdentifierCode;

	Ptr<CoreGraphics::ShaderState> mainState;

#if __NEBULA3_HTTP__
	Ptr<CoreGraphics::ShaderState> debugState; // instance of shader with all variables, only to be used by debugging
#endif
    Util::Array<Ptr<CoreGraphics::ShaderState>> shaderInstances;
    Ptr<CoreGraphics::ShaderVariation> activeVariation;
    bool inBeginUpdate;
	Util::StringAtom shaderName;

    Util::Dictionary<CoreGraphics::ShaderFeature::Mask, Ptr<CoreGraphics::ShaderVariation>> variations;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderState>&
ShaderBase::GetMainState()
{
	return this->mainState;
}

#if __NEBULA3_HTTP__
//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderState>&
ShaderBase::GetDebugState()
{
	return this->debugState;
}
#endif

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<CoreGraphics::ShaderState> >&
ShaderBase::GetAllShaderStates() const
{
    return this->shaderInstances;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom 
ShaderBase::GetShaderName() const
{
	return this->shaderName;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::ShaderIdentifier::Code&
ShaderBase::GetCode() const
{
    return this->shaderIdentifierCode;
}
//------------------------------------------------------------------------------
/**
*/
inline SizeT
ShaderBase::GetNumVariations() const
{
    return this->variations.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderVariation>&
ShaderBase::GetVariationByIndex(IndexT i) const
{
    return this->variations.ValueAtIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ShaderBase::HasVariation(CoreGraphics::ShaderFeature::Mask featureMask) const
{
    return this->variations.Contains(featureMask);
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderVariation>&
ShaderBase::GetVariationByFeatureMask(CoreGraphics::ShaderFeature::Mask featureMask) const
{
    return this->variations[featureMask];
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderVariation>&
ShaderBase::GetActiveVariation() const
{
    return this->activeVariation;
}

} // namespace Base
//------------------------------------------------------------------------------

