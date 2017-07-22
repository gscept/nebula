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

	/// register entity
	Graphics::ContextId Register(const Graphics::EntityId entity, const Resources::ResourceName& modelName);
	/// unregister entity
	void Unregister(const Graphics::EntityId entity);

	/// change model for existing entity
	void ChangeModel(const Graphics::ContextId id, const Resources::ResourceName& modelName);

	/// called from view to resolve visibility for entities
	void OnResolveVisibility(IndexT frameIndex, bool updateLod = false);
	/// called before culling takes place
	void OnCullBefore(Timing::Time time, Timing::Time globalTimeFactor, IndexT frameIndex);
	/// called when culling notifies entities are visible
	void OnNotifyCullingVisible(const Graphics::EntityId observer, IndexT frameIndex);
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
		Ptr<Materials::SurfaceInstance> surface;
	};
private:

	/// create model instance, which is essentially a list of instances of the nodes it consists of
	ModelInstanceId CreateModelInstance(const Resources::ResourceId id);
	/// get model instance as list of nodes
	const Util::Array<NodeInstance>& GetNodes(const ModelInstanceId id);

	Ids::IdGenerationPool modelInstancePool;
	Ids::IdGenerationPool nodeInstancePool;
	Util::Array<Util::Array<NodeInstance>> modelInstances;

	Util::Array<Resources::ResourceId> modelResources;
	Util::Array<ModelInstanceId> modelInstanceIds;
};

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<ModelContext::NodeInstance>&
ModelContext::GetNodes(const ModelInstanceId id)
{
	n_assert(this->modelInstancePool.IsValid(id));
	return this->modelInstances[Ids::Index(id)];
}

//------------------------------------------------------------------------------
/**
	Shorthand for adding a model to a graphics entity, Models::Attach
*/
inline Graphics::ContextId
Attach(const Graphics::EntityId entity, const Resources::ResourceName& res)
{
	ModelContext::Instance()->Register(entity, res);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Detach(const Graphics::EntityId entity)
{
	ModelContext::Instance()->Unregister(entity);
}

} // namespace Models