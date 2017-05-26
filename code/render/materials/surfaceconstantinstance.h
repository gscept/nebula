#pragma once
//------------------------------------------------------------------------------
/**
	@class Materials::SurfaceConstantInstance
	
	A surface constant instance works as a way to have a per-object surface override.
    Creating a surface constant instance allows it to be applied in a deferred manner,
    meaning we apply it after we apply the surface itself)
	
	(C) 2015 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/shaderstate.h"
#include "coregraphics/shadervariableinstance.h"
namespace Materials
{
class SurfaceConstant;
class SurfaceConstantInstance : public Core::RefCounted
{
	__DeclareClass(SurfaceConstantInstance);
public:
	/// constructor
	SurfaceConstantInstance();
	/// destructor
	virtual ~SurfaceConstantInstance();

    /// setup instance of constant
    void Setup(const Ptr<SurfaceConstant>& constant);
    /// discards instance of constant
    void Discard();

    /// returns true if constant is valid (basically checks if Discard hasn't been run yet)
    const bool IsValid() const;

    /// apply constant instance to shader
    void Apply(const Ptr<CoreGraphics::ShaderState>& shader);

    /// set value of constant, the actual shader value will only be applied whenever explicitly called with the Apply function (which should happen when rendering)
    void SetValue(const Util::Variant& value);
    /// set value shorthand for textures
    void SetTexture(const Ptr<CoreGraphics::Texture>& tex);

private:

    Ptr<SurfaceConstant> constant;
	Util::Dictionary<Ptr<CoreGraphics::ShaderState>, Ptr<CoreGraphics::ShaderVariableInstance>> variablesByShader;
};

//------------------------------------------------------------------------------
/**
*/
inline const bool
SurfaceConstantInstance::IsValid() const
{
    return this->constant != NULL;
}

} // namespace Materials