#pragma once
//------------------------------------------------------------------------------
/**
	The visibility server is the main hub for the visibility subsystem.

	The visibility server should be the called at the beginning of a frame, which
	fires off visibility queries for all observers.

	At some later point during the frame, each observer can obtain its list of graphics
	entities to render. 

	The visibility server has a set of visibility systems. All observers will iterate over 
	all visibility systems, however will skip entities previously visible from the chain of systems.
	In essence, a visibility system just manages the visible entities, like a Quad/Oct-tree, Boxes, or 
	BVH clusters. 

	The ObserverMask is used to tell the visibility subsystem what to register as visible,
	for example, a camera might see all geometry and lights, meanwhile lights are only interested
	in geometry (for shadow maps).

	All entities in all stages will be registered here, but each view will only contain check visibility
	for entities visible for that view. This way, a Stage is simply a visibility filter. Therefore, all entities, 
	if they are to be rendered, needs to be registered here. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "observer.h"
#include "graphics/graphicsentity.h"
#include "models/modelcontext.h"
#include "visibility.h"
#include "graphics/graphicscontext.h"

namespace CoreGraphics
{
class Mesh;
class PrimitiveGroup;
}

namespace Materials
{
class SurfaceName;
}
namespace Graphics
{
class View;
}
namespace Visibility
{

class VisibilitySystemBase;
class VisibilityContainer;
class VisibilityServer : public Core::RefCounted
{
	__DeclareClass(VisibilityServer);
	__DeclareSingleton(VisibilityServer);
public:
	/// constructor
	VisibilityServer();
	/// destructor
	virtual ~VisibilityServer();

	/// register visibility system
	//void RegisterVisibilitySystem(const Ptr<VisibilitySystemBase>& system);

	/// update visibility server - once per frame - walks through all observers and builds a visibility list
	void BeginVisibility();
	/// applies a visibility result, may apply the previous visibility result if the previously initiated is not ready yet, and makes that container the current one
	void ApplyVisibility(const Ptr<Graphics::View>& view);

	/// register graphics entity as an observer
	void RegisterObserver(const Graphics::GraphicsEntityId obs, ObserverMask mask);
	/// unregister graphics entity 
	void UnregisterObserver(const Graphics::GraphicsEntityId obs, ObserverMask mask);

	/// begin preparing for the scene to change, this phase should encompass the entirety of the void outside of the render loop
	void EnterVisibilityLockstep();
	/// register a renderable entity
	void RegisterGraphicsEntity(const Graphics::GraphicsEntityId entity);
	/// deregister renderable entity
	void UnregisterGraphicsEntity(const Graphics::GraphicsEntityId entity);
	/// finish up scene changes, this will cause all observers to reconstruct their visibility list
	void LeaveVisibilityLockstep();


private:

	bool locked;
	bool visibilityDirty;
	Util::Array<Graphics::GraphicsEntityId> observers;
	Util::Array<ObserverMask> observerMasks;
	//Util::Array<Ptr<VisibilitySystemBase>> systems;

	Util::Array<Graphics::GraphicsEntityId> entities;
	Util::Array<Models::ModelInstanceId> models;

	typedef Ids::Id32 MaterialTypeId;
	typedef CoreGraphics::BatchGroup::Code MaterialBatchId;
	typedef Ids::Id32 MaterialInstanceId;
	typedef Ids::Id32 ModelResourceId;
	typedef Ids::Id32 ModelNodeResourceId;
	typedef Ids::Id32 ModelNodeInstanceId;

	template <class KEY, class VALUE>
	struct VisibilityLevel
	{
		bool anyVisible;
		Util::HashTable<KEY, VALUE> level;
	};

	/// the visibility data structure, the entity id is the observer list
	typedef Util::HashTable<Graphics::GraphicsEntityId,
		VisibilityLevel<MaterialTypeId,
		VisibilityLevel<MaterialBatchId,
		VisibilityLevel<MaterialInstanceId,
		VisibilityLevel<ModelResourceId,
		VisibilityLevel<ModelNodeResourceId, Util::Array<ModelNodeInstanceId>
		>>>>>> VisibilityMatrix;
};
} // namespace Visibility