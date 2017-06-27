#pragma once
//------------------------------------------------------------------------------
/**
	The graphics server is the main singleton for the Graphics subsystem.

	Updating the GraphicsServer will progress the rendering process by one frame. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "framesync/framesynctimer.h"
#include "visibility/visibilityserver.h"
#include "util/stringatom.h"
namespace Graphics
{
class View;
class GraphicsServer : public Core::RefCounted
{
	__DeclareClass(GraphicsServer);
	__DeclareSingleton(GraphicsServer);
public:
	/// constructor
	GraphicsServer();
	/// destructor
	virtual ~GraphicsServer();

	/// opens the graphics server
	void Open();
	/// closes the graphics server
	void Close();

	/// create a new view
	Ptr<View> CreateView(const Util::StringAtom& framescript);
	/// discard view
	void DiscardView(const Ptr<View>& view);

	/// call per-frame to update graphics subsystem
	void OnFrame();

	/// create and register context class with graphics server, this later allows for that context to be used with graphics entities
	void RegisterGraphicsContext(const Core::Rtti& rtti);
private:
	friend class GraphicsEntity;

	/// register graphics entity with server
	void RegisterEntity(const Ptr<GraphicsEntity>& entity);
	/// unregister graphics entity with server
	void UnregisterEntity(const Ptr<GraphicsEntity>& entity);

	Memory::SliceAllocatorPool<Math::matrix44, 256, false> transforms;
	Util::Dictionary<int64_t, int64_t> entityTransformMap;
	Ptr<FrameSync::FrameSyncTimer> timer;
	Util::Array<Ptr<GraphicsContext>> contexts;
	Ptr<Visibility::VisibilityServer> visServer;

	Util::Array<Ptr<View>> views;

	bool isOpen;
};
} // namespace Graphics