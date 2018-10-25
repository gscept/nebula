#pragma once
//------------------------------------------------------------------------------
/**
    @class RenderModules::RTPlugin

    Standard interface to add new functionality to the render thread.
    Adding functionality to the render thread usually requires the
    following steps:

    - implement a set of classes which implement the render-thread
      functionality
    - implement proxy classes which act as a frontend in the main thread
    - implement a message protocol
    - implement a message handler
    - derive a new class from RenderThreadPlugin, setup an instance and
      call GraphicsInterface::Instance()->RegisterRenderThreadPlugin()

    Please note that the RenderThreadPlugin object lives completely on
    the render thread side! Define a clear separation line between
    main-thread and render-thread code and use messages to communicate
    between the two!

    NOTE: all "On*" methods are called from the RenderThread!

    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "timing/time.h"
#include "util/stringatom.h"

namespace Graphics
{
class Stage;
class View;
class GraphicsEntity;
};

//------------------------------------------------------------------------------
namespace RenderModules
{
class RTPlugin : public Core::RefCounted
{
    __DeclareClass(RTPlugin);
public:
    /// constructor
    RTPlugin();
    /// destructor
    virtual ~RTPlugin();

    /// called when plugin is registered on the render-thread side
    virtual void OnRegister();
    /// called when plugin is unregistered on the render-thread side
    virtual void OnUnregister();
    /// called when a new stage has been created
    virtual void OnStageCreated(const Ptr<Graphics::Stage>& stage);
    /// called when a stage is discarded
    virtual void OnDiscardStage(const Ptr<Graphics::Stage>& stage);
    /// called when a new view has been created
    virtual void OnViewCreated(const Ptr<Graphics::View>& view);
    /// called when a view is being discarded
    virtual void OnDiscardView(const Ptr<Graphics::View>& view);
    /// called when a graphics entity has been attached to a stage
    virtual void OnAttachEntity(const Ptr<Graphics::GraphicsEntity>& entity);
    /// called when a graphics entity is being removed from a stage
    virtual void OnRemoveEntity(const Ptr<Graphics::GraphicsEntity>& entity);
    /// called before updating entities
    virtual void OnUpdateBefore(IndexT frameId, Timing::Time time);
    /// called after updating entities
    virtual void OnUpdateAfter(IndexT frameId, Timing::Time time);
    /// called before rendering entities
    virtual void OnRenderBefore(IndexT frameId, Timing::Time time);
    /// called after rendering entities
    virtual void OnRenderAfter(IndexT frameId, Timing::Time time);
	/// called when rendering a specific subset of plugins
	virtual void OnRender(const Util::StringAtom& filter);
	/// called at the beginning of frame
	virtual void OnFrameBefore(IndexT frameId, Timing::Time time);
	/// called at the end of frame
	virtual void OnFrameAfter(IndexT frameId, Timing::Time time);
	/// called when rendering from special frame tag
	virtual void OnRenderFrame();
    /// called if no view exists, and no default camera is set in view
    virtual void OnRenderWithoutView(IndexT frameId, Timing::Time time);
	/// called if the window size has changed
	virtual void OnWindowResized(IndexT windowId, SizeT width, SizeT height);
};

} // namespace RenderModules
//------------------------------------------------------------------------------
