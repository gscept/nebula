#pragma once
//------------------------------------------------------------------------------
/**
	A graphics context is the base class which implements a graphics entity rendering component.
	This class is abstract, but any class inheriting it has to be a singleton.

	The idea is that every rendering module should implement its own GraphicsContext. The data
	for each graphics entity is then saved in a BlockAllocator as defined in this class, and
	that data is sliced into whatever is important. This way, we can simply loop over all
	slices in the BlockAllocator once per update, and simply ignore all the crud in between.

	Example:

		OOP:
			Entity:
				Model
				Character
				AnimEvents
				Resource Loading data
				Instancing stuff
				Picking stuff
				LightProbe stuff
				Transform

			For each entity
				Run anim event update on entity
					> requires: animation event list, character
					> stride between each entity = tons of stuff
		DO:
			Entity:
				Transform
			Context Singleton:
				Models[]
				Characters[]
				AnimEvents[]

			For each frame
				Run anim event update on AnimEvents[] array
					> Updates all entities
					> stride between each entity = 0

	This base class implements a block allocator which creates chunks of FixedPool objects.
	Whenever an entity is registered, a slice ID is calculated using the pool index, and the index within the pool as a 64 bit integer.
	The entity id is then paired with that slice id, so that the slice and pool can be obtained and free'd. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "timing/time.h"
#include "ids/id.h"
#include "ids/idgenerationpool.h"
#include "graphicsentity.h"
namespace Graphics
{
typedef Ids::Id32 ContextId;
class View;
class GraphicsContext : public Core::RefCounted
{
	__DeclareAbstractClass(GraphicsContext);

protected:
	/// constructor
	GraphicsContext();
	/// destructor
	virtual ~GraphicsContext();

	friend class GraphicsServer;

	/// runs before frame is updated
	void OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime);
	/// runs when visibility has finished processing 
	void OnVisibilityReady(const IndexT frameIndex, const Timing::Time frameTime);
	/// runs before a specific view
	void OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	/// runs after view is rendered
	void OnAfterView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	/// runs after a frame is updated
	void OnAfterFrame(const IndexT frameIndex, const Timing::Time frameTime);

protected:
	Util::Dictionary<int64_t, int64_t> entitySliceMap;
	Ids::IdGenerationPool contextIdPool;
};

} // namespace Graphics