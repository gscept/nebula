#pragma once
//------------------------------------------------------------------------------
/**
	@class Materials::ManagedSurface
	
	Implements a managed resource container for a surface resource.
	
	(C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/managedresource.h"
#include "materials/surface.h"
namespace Materials
{
class ManagedSurface : public Resources::ManagedResource
{
	__DeclareClass(ManagedSurface);
public:
	/// get contained surface material, or the placeholder material if material isn't loaded
    const Ptr<Surface>& GetSurface() const;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Surface>&
Materials::ManagedSurface::GetSurface() const
{
    return this->GetLoadedResource().downcast<Surface>();
}

} // namespace Materials