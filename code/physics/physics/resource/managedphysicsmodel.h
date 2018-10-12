#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::ManagedPhysicsModel
  
    Specialized managed resource for physics models
    
	(C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "resources/managedresource.h"
#include "physics/model/physicsmodel.h"

//------------------------------------------------------------------------------
namespace Physics
{
	class ManagedPhysicsModel : public Resources::ManagedResource
{
    __DeclareClass(ManagedPhysicsModel);
public:
    /// get contained mesh resource
    const Ptr<Physics::PhysicsModel>& GetModel() const;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Physics::PhysicsModel>&
ManagedPhysicsModel::GetModel() const
{
    return this->GetLoadedResource().downcast<Physics::PhysicsModel>();
}

} // namespace Physics
//------------------------------------------------------------------------------
