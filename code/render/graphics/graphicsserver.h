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
#include "util/stringatom.h"
#include "ids/idgenerationpool.h"
#include "graphicsentity.h"
namespace Graphics
{

class GraphicsContext;
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

	/// create graphics entity
	GraphicsEntityId CreateGraphicsEntity();
	/// discard graphics entity
	void DiscardGraphicsEntity(const GraphicsEntityId id);
	/// check if graphics entity is valid
	bool IsValidGraphicsEntity(const GraphicsEntityId id);

	/// create a new view
	Ptr<View> CreateView(const Util::StringAtom& framescript);
	/// discard view
	void DiscardView(const Ptr<View>& view);
	/// create a new stage
	Ptr<Stage> CreateStage(const Util::StringAtom& name, bool main);
	/// discard stage
	void DiscardStage(const Ptr<Stage>& stage);	

	/// call per-frame to update graphics subsystem
	void OnFrame();

	/// create and register context class with graphics server, this later allows for that context to be used with graphics entities
	void RegisterGraphicsContext(const Core::Rtti& rtti);
private:
	friend class GraphicsEntity;

	Ids::IdGenerationPool entityPool;
	Util::Array<GraphicsEntity> entities;

	Ptr<FrameSync::FrameSyncTimer> timer;
	Util::Array<Ptr<GraphicsContext>> contexts;
	//Ptr<Visibility::VisibilityServer> visServer;

	Util::Array<Ptr<Stage>> stages;
	Util::Array<Ptr<View>> views;

	bool isOpen;
};

//------------------------------------------------------------------------------
/**
*/
static GraphicsEntityId
CreateEntity()
{
	return GraphicsServer::Instance()->CreateGraphicsEntity();
}

//------------------------------------------------------------------------------
/**
*/
static void DestroyEntity(const GraphicsEntityId id)
{
	GraphicsServer::Instance()->DiscardGraphicsEntity(id);
}

} // namespace Graphics