#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::ShaderVariationBase
    
    A shader variation is part of a shader which implements a specific
    behaviour of the shader, identified by a set of "features". 
    Shader variations may implement a depth-only version of the shader,
    or geometry-deformed-versions of the shader like skinning or
    shape-blending. There is no pre-defined set of variation feature,
    this depends on the actually implemented render pipeline.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "coregraphics/shaderfeature.h"

//------------------------------------------------------------------------------
namespace Base
{
class ShaderVariationBase : public Core::RefCounted
{
    __DeclareClass(ShaderVariationBase);
public:
    typedef Util::StringAtom Name;
        
    /// constructor
    ShaderVariationBase();
    /// destructor
    virtual ~ShaderVariationBase();

	/// discard variation
	void Discard();
    
    /// get the shader variation's name
    const Name& GetName() const;
    /// get the feature bit mask of this variation
    CoreGraphics::ShaderFeature::Mask GetFeatureMask() const;
    /// get number of passes in this variation
    SizeT GetNumPasses() const;

protected:
    /// set variation name
    void SetName(const Name& n);
    /// set feature bit mask of this variation
    void SetFeatureMask(CoreGraphics::ShaderFeature::Mask m);
    /// set number of passes
    void SetNumPasses(SizeT n);

    Name name;
    CoreGraphics::ShaderFeature::Mask featureMask;
    SizeT numPasses;
};

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariationBase::SetName(const Name& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const ShaderVariationBase::Name&
ShaderVariationBase::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariationBase::SetFeatureMask(CoreGraphics::ShaderFeature::Mask m)
{
    this->featureMask = m;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::ShaderFeature::Mask
ShaderVariationBase::GetFeatureMask() const
{
    return this->featureMask;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariationBase::SetNumPasses(SizeT n)
{
    this->numPasses = n;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
ShaderVariationBase::GetNumPasses() const
{
    return this->numPasses;
}

} // namespace Base
//------------------------------------------------------------------------------


    