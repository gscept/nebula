//------------------------------------------------------------------------------
//  physicsserverbase.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physicsserver.h"
#include "resources/simpleresourcemapper.h"
#include "physics/resource/streamphysicsmeshloader.h"
#include "physics/resource/streamphysicsmodelloader.h"
#include "resources/resourcemanager.h"
#include "physics/physicsbody.h"
#include "physics/collider.h"
#include "physics/scene.h"
#include "physics/resource/managedphysicsmodel.h"
#include "debug/debugtimer.h"
//#include "physics/joints.h"
#include "physics/resource/physicsmesh.h"
#include "physics/visualdebuggerserver.h"
#include "io/assignregistry.h"
#include "../resource/managedphysicsmesh.h"

namespace Physics
{
__ImplementAbstractClass(Physics::BasePhysicsServer, 'PSRV', Core::RefCounted);
__ImplementInterfaceSingleton(Physics::BasePhysicsServer);

using namespace Math;

uint BasePhysicsServer::UniqueStamp = 0;

//------------------------------------------------------------------------------
/**
*/
BasePhysicsServer::BasePhysicsServer() :
    isOpen(false),
    time(0.0),
	simulationFrameTime(0.01666f),
	subSteps(1),
	initVisualDebugger(true)
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
BasePhysicsServer::~BasePhysicsServer()
{
    n_assert(!this->isOpen);
    n_assert(this->curScene== 0);
    
    __DestructInterfaceSingleton;

	this->visualDebuggerServer = 0;
}


//------------------------------------------------------------------------------
/**
    Set the current physics level. The refcount of the level will be 
    incremented, the refcount of the previous level will be decremented
    (if exists). A 0 pointer is valid and will just release the
    previous level.

    @param  level   pointer to a Physics::Level object
*/
void
BasePhysicsServer::SetScene(Scene * level)
{
    n_assert(this->isOpen);
    if (this->curScene.isvalid() != 0)
    {
        this->curScene->OnDeactivate();
        this->curScene = 0;
    }
    if (level)
    {
        this->curScene = level;
        this->curScene->OnActivate();
    }
}

//------------------------------------------------------------------------------
/**
    Return pointer to current level.

    @return     pointer to a Physics::Level object
*/
const Ptr<Scene> &
BasePhysicsServer::GetScene()
{
    return this->curScene;
}

//------------------------------------------------------------------------------
/**
    Initialize the physics subsystem.
*/
bool
BasePhysicsServer::Open()
{
    n_assert(!this->isOpen);

    // setup the material table
    MaterialTable::Setup();
	// setup resource mapper for physics resource files

	// assign special mesh resource prefix for physics meshes, which allows us to create duplicates.
	IO::AssignRegistry::Instance()->SetAssign(IO::Assign("phymsh", "export:meshes"));

	this->meshMapper = Resources::SimpleResourceMapper::Create();	
    this->meshMapper->SetResourceClass(PhysicsMesh::RTTI);
    this->meshMapper->SetResourceLoaderClass(StreamPhysicsMeshLoader::RTTI);
    this->meshMapper->SetManagedResourceClass(ManagedPhysicsMesh::RTTI);
    this->meshMapper->SetAsyncEnabled(false);
    Resources::ResourceManager::Instance()->AttachMapper(this->meshMapper.cast<Resources::ResourceMapper>());

    this->modelMapper = Resources::SimpleResourceMapper::Create();
    this->modelMapper->SetResourceClass(PhysicsModel::RTTI);
    this->modelMapper->SetResourceLoaderClass(StreamPhysicsModelLoader::RTTI);
    this->modelMapper->SetManagedResourceClass(ManagedPhysicsModel::RTTI);
    this->modelMapper->SetAsyncEnabled(false);
    Resources::ResourceManager::Instance()->AttachMapper(this->modelMapper.cast<Resources::ResourceMapper>());

    this->isOpen = true;

	_setup_timer(PhysicsTrigger)

    return true;
}

//------------------------------------------------------------------------------
/**
    Close the physics subsystem.
*/
void
BasePhysicsServer::Close()
{
	n_assert(this->isOpen);    
	Resources::ResourceManager::Instance()->RemoveMapper(PhysicsModel::RTTI);
	Resources::ResourceManager::Instance()->RemoveMapper(PhysicsMesh::RTTI);
    this->isOpen = false;
	_discard_timer(PhysicsTrigger)
}

//------------------------------------------------------------------------------
/**
    Perform one or more simulation steps. The number of simulation steps
    performed depends on the time of the last call to Trigger().
*/
void
BasePhysicsServer::Trigger()
{
	_start_timer(PhysicsTrigger)
    n_assert(this->isOpen);
    if (this->curScene != 0)
    {
        // trigger the level        
        this->curScene->Trigger();
    }

	_stop_timer(PhysicsTrigger)
}

//------------------------------------------------------------------------------
/**
    Renders the debug visualization of the level.
*/
void
BasePhysicsServer::RenderDebug()
{
    if (this->curScene != 0)
    {
        this->curScene->RenderDebug();
    }    
}

//------------------------------------------------------------------------------
/**
*/
void 
BasePhysicsServer::AddFilterSet(const Util::String & name, FilterSet* filter)
{
	this->filterSets.Add(name,filter);
}

//------------------------------------------------------------------------------
/**
*/
FilterSet *
BasePhysicsServer::GetFilterSet(const Util::String &name)
{
	n_assert2(this->filterSets.Contains(name),"unknown filterset, add it first\n");
	return this->filterSets.ValueAtIndex(this->filterSets.FindIndex(name));
}

//------------------------------------------------------------------------------
/**
*/
void 
BasePhysicsServer::AddCollisionReceiver(CollisionReceiver* rec)
{
	this->receivers.Append(rec);
}

//------------------------------------------------------------------------------
/**
*/
void 
BasePhysicsServer::RemoveCollisionReceiver(CollisionReceiver * rec)
{
	IndexT i = this->receivers.FindIndex(rec);
	n_assert2(i!=InvalidIndex, "try to remove non existing collisionreceiver");
	this->receivers.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
    This method computes a ray in 3d space thru the mouse position.
    It uses the projection and view matrix from the cam of the view set.

    @param  mousePos    2d screen position of mouse
    @param  scale       length of ray cast into the 3d world
*/
//Math::line 
//PhysicsServer::ComputeWorldMouseRay(const float2& mousePos, float length)
//{
//    n_assert(Graphics::GraphicsServer::HasInstance());
//    n_assert(this->graphicsView.isvalid());
//    const Ptr<Graphics::CameraEntity>& cam = this->graphicsView->GetCameraEntity();
//    n_assert(cam.isvalid());
//    const matrix44& view = matrix44::inverse(cam->GetViewTransform());
//    matrix44 invProj = matrix44::inverse(cam->GetProjTransform());
//
//    return Graphics::GraphicsServer::Instance()->ComputeWorldMouseRay(mousePos, length, view, invProj, cam->GetNearClipPlane());
//}


} // namespace Physics