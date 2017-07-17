#pragma once
//------------------------------------------------------------------------------
/**
	A model context bind a ModelInstance to a model, which allows it to be rendered using an .n3 file.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphicscontext.h"
#include "core/singleton.h"
#include "resources/resourceid.h"
namespace Graphics
{
class GraphicsEntity;
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
	ContextId Register(const EntityId entity, const Resources::ResourceName& modelName);
	/// unregister entity
	void Unregister(const EntityId entity);

	/// change model for existing entity
	void ChangeModel(const ContextId id, const Resources::ResourceName& modelName);

	/// called from view to resolve visibility for entities
	void OnResolveVisibility(IndexT frameIndex, bool updateLod = false);
	/// called before culling takes place
	void OnCullBefore(Timing::Time time, Timing::Time globalTimeFactor, IndexT frameIndex);
	/// called when culling notifies entities are visible
	void OnNotifyCullingVisible(const Ptr<GraphicsEntity>& observer, IndexT frameIndex);
	/// called just before rendering
	void OnRenderBefore(IndexT frameIndex);
	/// called to render debug visualizations
	void OnRenderDebug();

	/// called each frame to update the staging resources, when model is properly loaded, obtains actual ModelInstance
	void OnRefreshStagingResources();
private:

	Util::Array<Resources::ResourceId> modelResources;

};
} // namespace Graphics