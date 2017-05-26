#pragma once
//------------------------------------------------------------------------------
/**
	@class Materials::Surface

    A surface is a collection of shaders, and the parameters used for a surface.

    A surface material thus represents all shader constants present in all the shaders
    its material template is implementing. This makes a surface material reusable and shareable
    over several models, and also exchangeable without having to modify the model resource.

    The SurfaceMaterial class is not responsible for applying the shaders themselves, this is
    handled by the FrameBatch, but each state node instance should have a surface material.
    If a surface material fails to load, it is assigned the placeholder material instead.

    Surfaces are cloneable, which makes it possible to have per-instance unique materials.
    A clone will copy the original surface, but will also allow for constants to be set
    on a per-object basis. Since SurfaceMaterial is a resource, it is only ever truly destroyed
    once all instances of the material (and its clones) gets discarded.

    However surface constants are also instanciateable, which means one can, 
    without duplicating a material, have a variable which overrides the original.
    Cloning a material is useful if one wishes to make a entirely new material
    from an existing one, without the overhead of having per-instance constant
    overrides.
	
	(C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resource.h"
#include "material.h"
#include "physics/materialtable.h"
#include "materials/materialtype.h"
#include "surfaceconstant.h"
#include "surfacename.h"
namespace Materials
{
class SurfaceInstance;
class Surface : public Resources::Resource
{
	__DeclareClass(Surface);
public:
	/// constructor
	Surface();
	/// destructor
	virtual ~Surface();

    /// discard surface
    void Discard();
    /// unload surface
    void Unload();
    /// reload the surface
    void Reload();

    /// create an instance of a surface
    Ptr<SurfaceInstance> CreateInstance();
    /// discard an instance of a surface
    void DiscardInstance(const Ptr<SurfaceInstance>& instance);

    /// get original material
    const Ptr<Materials::Material>& GetMaterialTemplate();

    /// get material type (from original material)
    const SurfaceName::Code& GetSurfaceCode() const;

protected:
    friend class SurfaceInstance;
    friend class StreamSurfaceLoader;
	friend class StreamSurfaceSaver;

    /// setup surface from original material
    void Setup(const Ptr<Material>& material);

    struct SurfaceValueBinding
    {
        Util::Variant value;
        bool system;
    };

    Util::Dictionary<Util::StringAtom, SurfaceValueBinding> staticValues;
    Util::Array<Ptr<SurfaceInstance>> instances;
    Ptr<Material> materialTemplate;
    SurfaceName::Code code;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Materials::Material>&
Surface::GetMaterialTemplate()
{
    return this->materialTemplate;
}

//------------------------------------------------------------------------------
/**
*/
inline const SurfaceName::Code&
Surface::GetSurfaceCode() const
{
    return this->code;
}

} // namespace Materials