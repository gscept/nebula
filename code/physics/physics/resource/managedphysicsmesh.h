#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::ManagedPhysicsMesh
  
    Specialized managed resource for physics meshes.
    
	(C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "resources/managedresource.h"
#include "physics/resource/physicsmesh.h"

//------------------------------------------------------------------------------
namespace Physics
{
	class ManagedPhysicsMesh : public Resources::ManagedResource
{
    __DeclareClass(ManagedPhysicsMesh);
public:
    /// get contained mesh resource
    const Ptr<Physics::PhysicsMesh>& GetMesh() const;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Physics::PhysicsMesh>&
ManagedPhysicsMesh::GetMesh() const
{
    return this->GetLoadedResource().downcast<Physics::PhysicsMesh>();
}

} // namespace Physics
//------------------------------------------------------------------------------
