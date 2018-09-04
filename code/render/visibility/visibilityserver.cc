//------------------------------------------------------------------------------
// visibilityserver.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "visibilityserver.h"

namespace Visibility
{

__ImplementClass(Visibility::VisibilityServer, 'VISE', Core::RefCounted);
__ImplementSingleton(Visibility::VisibilityServer);
//------------------------------------------------------------------------------
/**
*/
VisibilityServer::VisibilityServer() :
	locked(false)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VisibilityServer::~VisibilityServer()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
void
VisibilityServer::RegisterVisibilitySystem(const Ptr<VisibilitySystemBase>& system)
{
	n_assert(this->systems.FindIndex(system) == InvalidIndex);
	this->systems.Append(system);
}
*/

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::BeginVisibility()
{
	// create visibility jobs for all observers
	IndexT i;
	for (i = 0; i < this->observers.Size(); i++)
	{
		//Ptr<Jobs::Job> jobs = Jobs::Job::Create();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::ApplyVisibility(const Ptr<Graphics::View>& view)
{

}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::EnterVisibilityLockstep()
{
	n_assert(!this->locked);
	this->locked = true;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityServer::LeaveVisibilityLockstep()
{
	n_assert(this->locked);
	this->locked = false;
}

} // namespace Visibility