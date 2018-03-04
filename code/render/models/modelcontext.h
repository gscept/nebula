#pragma once
//------------------------------------------------------------------------------
/**
	A model context bind a ModelInstance to a model, which allows it to be rendered using an .n3 file.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "core/singleton.h"
#include "resources/resourceid.h"
#include "models/modelserver.h"
#include "materials/materialserver.h"
namespace Models
{
typedef Ids::Id32 ModelInstanceId;
class ModelContext : public Graphics::GraphicsContext
{
	__DeclareClass(ModelContext);
	__DeclareSingleton(ModelContext);
public:
	/// constructor
	ModelContext();
	/// destructor
	virtual ~ModelContext();

	/// setup
	void Setup(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name);

	/// change model for existing entity
	void ChangeModel(const Graphics::GraphicsEntityId id, const Resources::ResourceName& modelName);
	/// get model
	const Models::ModelInstanceId GetModel(const Graphics::GraphicsEntityId id);

	/// called from view to resolve visibility for entities
	void OnResolveVisibility(IndexT frameIndex, bool updateLod = false);
	/// called before culling takes place
	void OnCullBefore(Timing::Time time, Timing::Time globalTimeFactor, IndexT frameIndex);
	/// called when culling notifies entities are visible
	void OnNotifyCullingVisible(const Graphics::GraphicsEntityId observer, IndexT frameIndex);
	/// called just before rendering
	void OnRenderBefore(IndexT frameIndex);
	/// called to render debug visualizations
	void OnRenderDebug();

	/// called each frame to update the staging resources, when model is properly loaded, obtains actual ModelInstance
	void OnRefreshStagingResources();

	typedef IndexT NodeInstanceId;

	/// this structure is really only used for modifying
	struct NodeInstance
	{
		NodeInstanceId id;
		Math::matrix44* parent;
		Math::matrix44 transform;
		Resources::ResourceId mesh;
		IndexT primitiveGroupId;
		Materials::MaterialId material;
	};
private:

	/// create model instance, which is essentially a list of instances of the nodes it consists of
	ModelInstanceId CreateModelInstance(const Resources::ResourceId id);
	/// get model instance as list of nodes
	const Util::Array<NodeInstance>& GetNodes(const ModelInstanceId id);

	Ids::IdAllocator<
		Resources::ResourceId,
		ModelInstanceId,
		Util::Array<NodeInstance>
	> modelContextAllocator;

	Ids::IdAllocator<
		Models::ModelId,			// parent
		Math::matrix44,				// transform
		Util::Array<NodeInstance>
	> modelInstanceAllocator;

	/// allocate a new slice for this context
	Ids::Id32 Alloc();
	/// deallocate a slice
	void Dealloc(Ids::Id32 id);

};

//------------------------------------------------------------------------------
/**
*/
inline Ids::Id32
ModelContext::Alloc()
{
	return this->modelContextAllocator.AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelContext::Dealloc(Ids::Id32 id)
{
	this->modelContextAllocator.DeallocObject(id);
}

//------------------------------------------------------------------------------
/**
	Shorthand for adding a model to a graphics entity, Models::Attach
*/
inline void
ModelContextAttach(const Graphics::GraphicsEntityId entity, const Resources::ResourceName& res)
{
	ModelContext::Instance()->RegisterEntity(entity);
	ModelContext::Instance()->Setup(entity, res);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelContextDetach(const Graphics::GraphicsEntityId entity)
{
	ModelContext::Instance()->DeregisterEntity(entity);
}

} // namespace Models