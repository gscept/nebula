#pragma once
//------------------------------------------------------------------------------
/**
	@class Materials::SurfaceInstance
	
	Implements an instance (and shader stage mutable) instance of a surface.

    Upon creation, this will use the shader variables contained within the surface,
    however, has its own local state which can be muted. However, the surface this uses
    will still use the same shaders
	
	(C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "surfaceconstant.h"
#include "util/stringatom.h"
#include "surfacename.h"
#include "graphics/batchgroup.h"
namespace Materials
{
class Surface;
class SurfaceInstance : public Core::RefCounted
{
	__DeclareClass(SurfaceInstance);
public:
	/// constructor
	SurfaceInstance();
	/// destructor
	virtual ~SurfaceInstance();

    /// setup from a surface
    void Setup(const Ptr<Surface>& surface);
    /// discard this instance
    void Discard();

    /// cleanup the internal state of this instance
    void Cleanup();

    /// apply surface based on pass
	const CoreGraphics::ShaderStateId GetShaderState(const IndexT passIndex);

    /// set pre-defined value of a parameter, submit the true as the last argument if the material should change immediately
    void SetValue(const Util::StringAtom& param, const Util::Variant& value);
    /// set texture directly, shorthand for setvalue with a texture
    void SetTexture(const Util::StringAtom& param, const CoreGraphics::TextureId tex);
    /// set a managed texture (which may load asynchronously), which is then retrieved each frame
    void SetTexture(const Util::StringAtom& param, const Resources::ResourceId tex);

    /// returns true if surface has a constant with the given name
    bool HasConstant(const Util::StringAtom& name) const;
    /// return pointer to surface constant depending on name
    const Ptr<SurfaceConstant>& GetConstant(const Util::StringAtom& name) const;

    /// get code of surface (which resides within the original surface)
    const Materials::SurfaceName::Code& GetCode() const;

    /// apply the surface variables, but only for a specific shader instance
	void Apply(const IndexT passIndex);

protected:
    friend class Surface;

    Ptr<Surface> originalSurface;

    Materials::SurfaceName::Code code;
    Util::Array<Ptr<SurfaceConstant>> constants;
    Util::Dictionary<Util::StringAtom, Ptr<SurfaceConstant>> constantsByName;
    Util::Array<Ptr<CoreGraphics::ShaderState>> shaderInstances;
	Util::Dictionary<CoreGraphics::ShaderId, CoreGraphics::ShaderStateId> shaderInstancesByShader;
	Util::Array<Resources::ResourceId> allocatedTextures;
    struct DeferredTextureBinding
    {
    public:
		Resources::ResourceId tex;
        Util::StringAtom var;
    };

    Util::Array<DeferredTextureBinding> managedTextures;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
SurfaceInstance::HasConstant(const Util::StringAtom& name) const
{
    return this->constantsByName.Contains(name);

}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<SurfaceConstant>&
SurfaceInstance::GetConstant(const Util::StringAtom& name) const
{
    return this->constantsByName[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const Materials::SurfaceName::Code&
SurfaceInstance::GetCode() const
{
    return this->code;
}

} // namespace Materials