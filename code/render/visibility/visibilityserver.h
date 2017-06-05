#pragma once
//------------------------------------------------------------------------------
/**
	The visibility server is the main hub for the visibility subsystem.

	The visibility server should be the called at the beginning of a frame, which
	fires off visibility queries for all observers.

	At some later point during the frame, each observer can obtain its list of graphics
	entities to render. 

	The ObserverMask is used to tell the visibility subsystem what to register as visible,
	for example, a camera might see all geometry and lights, meanwhile lights are only interested
	in geometry (for shadow maps).

	All entities in all stages will be registered here, but each view will only contain check visibility
	for entities visible for that view. This way, a Stage is simply a visibility filter. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "observer.h"
#include "graphics/graphicsentity.h"
#include "graphics/modelcontext.h"

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

enum class ObserverMask : uint8_t
{
	Geometry	= (1 << 0),
	Lights		= (1 << 1)
};
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

	/// update visibility server - once per frame - walks through all observers and builds a visibility list
	void BeginVisibility();
	/// applies a visibility result, may apply the previous visibility result if the previously initiated is not ready yet, and makes that container the current one
	void ApplyVisibility(const Ptr<Graphics::View>& view);

	/// register graphics entity as an observer
	void RegisterObserver(const Ptr<Graphics::GraphicsEntity>& obs, ObserverMask mask);
	/// unregister graphics entity 
	void UnregisterObserver(const Ptr<Graphics::GraphicsEntity>& obs, ObserverMask mask);

	/// begin preparing for the scene to change, this phase should encompass the entirety of the void outside of the render loop
	void EnterVisibilityLockstep();
	/// register a renderable entity
	void RegisterGraphicsEntity(const Ptr<Graphics::GraphicsEntity>& entity, Graphics::ModelContext::_ModelResult* data);
	/// deregister renderable entity
	void UnregisterGraphicsEntity(const Ptr<Graphics::GraphicsEntity>& entity);
	/// finish up scene changes, this will cause all observers to reconstruct their visibility list
	void LeaveVisibilityLockstep();

	using SurfaceMeshNodeDatabase = Util::Dictionary<Materials::SurfaceName, Util::Dictionary<Ptr<CoreGraphics::Mesh>, Util::Array<CoreGraphics::PrimitiveGroup>>>;
private:

	bool locked;
	bool visibilityDirty;
	Util::Array<Ptr<Observer>> observers;
	Util::Array<Ptr<Graphics::GraphicsEntity>> entities;
	Util::Array<Graphics::ModelContext::_ModelResult*> models;
	SurfaceMeshNodeDatabase visibilityDatabase;
	Ptr<VisibilityContainer> activeVisibility;
};
} // namespace Visibility