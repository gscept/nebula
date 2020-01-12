#pragma once
//------------------------------------------------------------------------------
/**
	The graphics server is the main singleton for the Graphics subsystem.

	Updating the GraphicsServer will progress the rendering process by one frame. 
	
	(C)2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "framesync/framesynctimer.h"
#include "util/stringatom.h"
#include "ids/idgenerationpool.h"
#include "coregraphics/batchgroup.h"
#include "stage.h"
#include "graphicsentity.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/transformdevice.h"
#include "coregraphics/shaperenderer.h"
#include "coregraphics/textrenderer.h"
#include "frame/frameserver.h"
#include "debug/debughandler.h"
#include "materials/materialserver.h"

namespace Graphics
{

struct FrameContext
{
	Timing::Time time;
	Timing::Tick ticks;
	Timing::Time frameTime;
	IndexT frameIndex;
};

class GraphicsContext;
struct GraphicsContextFunctionBundle;
struct GraphicsContextState;
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

	/// create a new view with a new framescript
	Ptr<View> CreateView(const Util::StringAtom& name, const IO::URI& framescript);
	/// create a new view without a framescript
	Ptr<View> CreateView(const Util::StringAtom& name);
	/// discard view
	void DiscardView(const Ptr<View>& view);
	/// get current view
	const Ptr<View>& GetCurrentView() const;
	/// set current view (do not use unless you know what you are doing since this is normally handled by the graphicssserver)
	void SetCurrentView(const Ptr<View>& view);

	/// create a new stage
	Ptr<Stage> CreateStage(const Util::StringAtom& name, bool main);
	/// discard stage
	void DiscardStage(const Ptr<Stage>& stage);	

	/// call to prepare for the frame
	void BeginFrame();
	/// call before rendering to a view
	void BeforeViews();
	/// go through and render views
	void RenderViews();
	/// end views
	void EndViews();
	/// call when ending the frame
	void EndFrame();

	/// get total time in seconds
	const Timing::Time GetTime() const;
	/// get frame time in seconds
	const Timing::Time GetFrameTime() const;
	/// get frame index
	const IndexT GetFrameIndex() const;

    /// debug rendering
    void RenderDebug(uint32_t flags);

	/// register function bundle from graphics context, see GraphicsContextType::Create
	void RegisterGraphicsContext(GraphicsContextFunctionBundle* context, GraphicsContextState* state);
	/// unregister function bundle
	void UnregisterGraphicsContext(GraphicsContextFunctionBundle* context);
    
    /// call when the window has been resized
    void OnWindowResized(CoreGraphics::WindowId wndId);
private:
	friend class GraphicsEntity;
	friend class CoreGraphics::BatchGroup;

	Ids::IdGenerationPool entityPool;

	Ptr<FrameSync::FrameSyncTimer> timer;
	FrameContext frameContext;

	Util::Array<GraphicsContextFunctionBundle*> contexts;
	Util::Array<GraphicsContextState*> states;

	Util::Array<Ptr<Stage>> stages;
	Util::Array<Ptr<View>> views;
	CoreGraphics::BatchGroup batchGroupRegistry;
	Ptr<View> currentView;

	Ptr<CoreGraphics::DisplayDevice> displayDevice;
	bool graphicsDevice;
	Ptr<CoreGraphics::ShaderServer> shaderServer;
	Ptr<Materials::MaterialServer> materialServer;
	Ptr<CoreGraphics::TransformDevice> transformDevice;
	Ptr<CoreGraphics::ShapeRenderer> shapeRenderer;
	Ptr<CoreGraphics::TextRenderer> textRenderer;
	Ptr<Frame::FrameServer> frameServer;	

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
static void
DestroyEntity(const GraphicsEntityId id)
{
	GraphicsServer::Instance()->DiscardGraphicsEntity(id);
}

//------------------------------------------------------------------------------
/**
*/
template<typename ... CONTEXTS>
static void
RegisterEntity(const GraphicsEntityId id)
{
	(CONTEXTS::RegisterEntity(id), ...);
}

//------------------------------------------------------------------------------
/**
*/
template<typename ... CONTEXTS>
static void
DeregisterEntity(const GraphicsEntityId id)
{
	(CONTEXTS::DeregisterEntity(id), ...);
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<View>&
GraphicsServer::GetCurrentView() const
{
	return this->currentView;
}

//------------------------------------------------------------------------------
/**
*/
inline const Timing::Time 
GraphicsServer::GetTime() const
{
	return this->frameContext.time;
}

//------------------------------------------------------------------------------
/**
*/
inline const Timing::Time
GraphicsServer::GetFrameTime() const
{
	return this->frameContext.frameTime;
}

//------------------------------------------------------------------------------
/**
*/
inline const IndexT
GraphicsServer::GetFrameIndex() const
{
	return this->frameContext.frameIndex;
}

} // namespace Graphics