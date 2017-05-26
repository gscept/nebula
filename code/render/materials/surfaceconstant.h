#pragma once
//------------------------------------------------------------------------------
/**
	@class Materials::SurfaceConstant
	
	A surface constant is a value which represents a value constant to a specific type of surface.
    When rendering the same object using multiple shader, for example through shadow mapping, 
    deferred rendering and picking, we will want to have the ability to set a single constant
    which will be globally spread over all shaders. A surface constant is constant during
    the execution of the rendering, thus the name.
    
    An example for this is the alpha threshold when doing alpha clipping, 
    which is probably most intuitively seen when doing ordinary color shading, but is also
    relevant when clipping away pixels which should not be affected in the shadow map.

    SurfaceConstant needs a shader instance to apply to, so as to not apply to all
    shaders when only one is required.
	
	(C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "util/variant.h"
#include "coregraphics/texture.h"
#include "resources/managedtexture.h"
#include "graphics/batchgroup.h"
#include "material.h"

namespace CoreGraphics
{
class ShaderState;
class ShaderVariableInstance;
}

namespace Materials
{
class SurfaceConstant : public Core::RefCounted
{
	__DeclareClass(SurfaceConstant);
public:

	struct ConstantBinding
	{
		bool active;
		Ptr<CoreGraphics::ShaderVariable> var;
		Ptr<CoreGraphics::ShaderState> shd;
	};
	/// constructor
	SurfaceConstant();
	/// destructor
	virtual ~SurfaceConstant();

    /// set value of constant, the actual shader value will only be applied whenever explicitly called with the Apply function (which should happen when rendering)
    void SetValue(const Util::Variant& value);
    /// set value shorthand for textures
    void SetTexture(const Ptr<CoreGraphics::Texture>& tex);
    /// get value
    const Util::Variant& GetValue() const;
	/// returns true if constant is supposed to be system managed
	const bool IsSystemManaged() const;

    /// applies this constant, which readies it for drawing, but only applies the value on one of the shaders
    void Apply(const IndexT passIndex);

protected:
    friend class StreamSurfaceSaver;
    friend class SurfaceInstance;
    friend class Surface;

    /// setup constant, which initializes its name and bindings to its implementing shaders
	void Setup(const Util::StringAtom& name, const Util::Array<Material::MaterialPass>& passes, const Util::Array<Ptr<CoreGraphics::ShaderState>>& shaders);
    /// discard constant
    void Discard();

	/// set shader variable using variant
	void ApplyToShaderVariable(const Util::Variant& value, const Ptr<CoreGraphics::ShaderVariable>& var);

    bool system;
    Util::StringAtom name;
    Util::Variant value;
	Util::Array<ConstantBinding> bindingsByIndex;
};

//------------------------------------------------------------------------------
/**
*/
inline const Util::Variant&
SurfaceConstant::GetValue() const
{
    return this->value;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
SurfaceConstant::IsSystemManaged() const
{
	return this->system;
}

} // namespace Materials