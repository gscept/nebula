#pragma once
//------------------------------------------------------------------------------
/**
	Contains two contexts, one for observers and one for observables

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "visibility.h"
#include "jobs/jobs.h"
#include "visibility/systems/visibilitysystem.h"
namespace Visibility
{

class ObserverContext : public Graphics::GraphicsContext
{
	DeclareContext();
public:

	/// setup entity
	static void Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType entityType);

	/// runs before frame is updated
	static void OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime);

	/// create context
	static void Create();

	/// create a box system
	static VisibilitySystem* CreateBoxSystem(const BoxSystemLoadInfo& info);
	/// create a portal system
	static VisibilitySystem* CreatePortalSystem(const PortalSystemLoadInfo& info);
	/// create octree system
	static VisibilitySystem* CreateOctreeSystem(const OctreeSystemLoadInfo& info);
	/// create quadtree system
	static VisibilitySystem* CreateQuadtreeSystem(const QuadtreeSystemLoadInfo& info);
	/// create brute force system
	static VisibilitySystem* CreateBruteforceSystem(const BruteforceSystemLoadInfo& info);

	/// wait for all visibility jobs
	static void WaitForVisibility(const IndexT frameIndex, const Timing::Time frameTime);

	static Jobs::JobPortId jobPort;
	static Threading::SafeQueue<Jobs::JobId> runningJobs;

private:

	friend class ObservableContext;
	friend class VisibilityContex;
	typedef Ids::IdAllocator<
		bool,					// visibility result
		Ids::Id32,				// parent object
		ubyte					// number of nodes (max 255)
	> VisibilityResultAllocator;
	typedef Ids::IdAllocator<
		Math::matrix44,					// transform of observer camera
		Graphics::GraphicsEntityId, 	// entity id
		VisibilityEntityType,			// type of object so we know how to get the transform
		VisibilityResultAllocator		// visibility lookup table
	> ObserverAllocator;
	static ObserverAllocator observerAllocator;

	/// allocate a new slice for this context
	static Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(Graphics::ContextEntityId id);

	/// keep as ordinary array of pointers, no need to have them cache aligned
	static Util::Array<VisibilitySystem*> systems;
};

class ObservableContext : public Graphics::GraphicsContext
{
	DeclareContext();
public:

	/// setup entity
	static void Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType entityType);

	/// create context
	static void Create();
private:

	friend class ObserverContext;
	friend class VisibilityContex;
	typedef Ids::IdAllocator<
		Math::matrix44,					// transform
		Graphics::GraphicsEntityId,		// entity id
		VisibilityEntityType			// type of object so we know how to get the transform
	> ObserveeAllocator;
	static ObserveeAllocator observeeAllocator;

	/// allocate a new slice for this context
	static Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(Graphics::ContextEntityId id);
};

} // namespace Visibility
