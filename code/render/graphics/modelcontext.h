#pragma once
//------------------------------------------------------------------------------
/**
	A model context bind a ModelInstance to a model, which allows it to be rendered using an .n3 file.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphicscontext.h"
#include "models/modelinstance.h"
#include "core/singleton.h"
#include "resource/resourceid.h"
#include "resource/resourcecontainer.h"
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

	struct _ModelSetup
	{
		Resources::ResourceId res;
		bool synced;
	};

	struct _ModelResult
	{
		Ptr<Models::ModelInstance> model;
	};

	/// register entity
	_ModelResult* RegisterEntity(const Ptr<GraphicsEntity>& entity, _ModelSetup setup);
	/// unregister entity
	void UnregisterEntity(const Ptr<GraphicsEntity>& entity);

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

	struct _Staging
	{
		Ptr<Resources::ResourceContainer<Models::Model>> res;
	};

	GraphicsContext::BlockAllocator<_ModelResult> modelData;
	GraphicsContext::BlockAllocator<_Staging> stagingData;
	GraphicsContext::BlockAllocator<_ModelSetup> setupData;
};
} // namespace Graphics