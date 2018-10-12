#pragma once
//------------------------------------------------------------------------------
/**
    @class Input::InputHandler
    
    Input handlers receive and process input events. Handlers are chained 
    together, sorted by priority, and input events travel from one
    handler to the next. Input events may be blocked by an input handler,
    so that the blocked events are not passed on to the next lower-priority
    handlers. Subclasses of InputHandler present the received input in
    more specific ways.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "input/inputevent.h"

namespace Base
{
    class InputServerBase;
}

//------------------------------------------------------------------------------
namespace Input
{
class InputHandler : public Core::RefCounted
{
    __DeclareClass(InputHandler);
public:
    /// constructor
    InputHandler();
    /// destructor
    virtual ~InputHandler();
    
    /// return true if the input handler is currently attached
    bool IsAttached() const;
    /// capture input to this event handler
    virtual void BeginCapture();
    /// end input capturing to this event handler
    virtual void EndCapture();
    /// return true if this input handler captures input
    bool IsCapturing() const;

protected:
    friend class Base::InputServerBase;

    /// called when the handler is attached to the input server
    virtual void OnAttach();
    /// called when the handler is removed from the input server
    virtual void OnRemove();
    /// called on InputServer::BeginFrame()
    virtual void OnBeginFrame();
    /// called on InputServer::EndFrame();
    virtual void OnEndFrame();
    /// called when input handler obtains capture
    virtual void OnObtainCapture();
    /// called when input handler looses capture
    virtual void OnReleaseCapture();
    /// called when an input event should be processed
    virtual bool OnEvent(const InputEvent& inputEvent);
    /// called when the handler should reset itself
    virtual void OnReset();

    bool isAttached;
    bool isCapturing;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
InputHandler::IsAttached() const
{
    return this->isAttached;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
InputHandler::IsCapturing() const
{
    return this->isCapturing;
}

} // namespace Input
//------------------------------------------------------------------------------

