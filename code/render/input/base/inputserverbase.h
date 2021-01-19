#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::InputServerBase
    
    The InputServer is the central object of the Input subsystem. It 
    mainly manages a prioritized list of input handlers which process
    incoming input events.
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "input/inputpriority.h"
#include "input/inputevent.h"
#include "threading/criticalsection.h"   
#include "threading/thread.h"

namespace Input
{
    class Mouse;
    class Keyboard;
    class GamePad;
    class InputHandler;
}    

//------------------------------------------------------------------------------
namespace Base
{
class InputServerBase : public Core::RefCounted
{
    __DeclareClass(InputServerBase);
public:
    /// constructor
    InputServerBase();
    /// destructor
    virtual ~InputServerBase();

    /// set the max number of local players for this application (default is 4)
    void SetMaxNumLocalPlayers(SizeT maxNumLocalPlayers);
    /// get the max number of local players
    SizeT GetMaxNumLocalPlayers() const;
    /// open the input server
    virtual void Open();
    /// close the input server
    virtual void Close();
    /// return true if open
    bool IsOpen() const;
    /// set the quit requested flag
    void SetQuitRequested(bool b);
    /// return true if some subsystem has requested to quit the app (e.g. Alt-F4)
    bool IsQuitRequested() const;
    /// reset input state
    void Reset();
    /// get the default keyboard input handler
    const Ptr<Input::Keyboard>& GetDefaultKeyboard() const;
    /// get the default mouse input handler
    const Ptr<Input::Mouse>& GetDefaultMouse() const;
    /// get default gamepad handler (playerIndex is valid up to MaxNumLocalPlayers)
    Ptr<Input::GamePad> GetDefaultGamePad(IndexT playerIndex) const;

    /// attach an input handler
    void AttachInputHandler(Input::InputPriority::Code pri, const Ptr<Input::InputHandler>& inputHandler);
    /// remove an input handler
    void RemoveInputHandler(const Ptr<Input::InputHandler>& inputHandler);
    ///
    void AddGamePad(const Ptr<Input::GamePad> pad, IndexT playerIndex);

    /// call before processing window events
    virtual void BeginFrame();
    /// call after processing window events
    virtual void OnFrame();
    /// call at end of frame
    void EndFrame();
    /// put an input event into the handler chain
    void PutEvent(const Input::InputEvent& ie);

    /// clear the current mouse capture (if exists)
    void ClearMouseCapture();
    /// clear the current keyboard capture (if exists)
    void ClearKeyboardCapture();
    /// clear both mouse and keyboard captures
    void ClearCapture();
    /// return the current mouse capture input handler (return invalid ptr if no capture set)
    const Ptr<Input::InputHandler>& GetMouseCaptureHandler() const;
    /// return the current keyboard capture input handler (return invalid ptr if no capture set)
    const Ptr<Input::InputHandler>& GetKeyboardCaptureHandler() const;

    /// only call from InputHandler: capture mouse input to the given input handler
    void ObtainMouseCapture(const Ptr<Input::InputHandler>& inputHandler);
    /// only call from InputHandler: release mouse capture
    void ReleaseMouseCapture(const Ptr<Input::InputHandler>& inputHandler);
    /// only call from InputHandler: capture keyboard input to the given input handler
    void ObtainKeyboardCapture(const Ptr<Input::InputHandler>& inputHandler);
    /// only call from InputHandler: release keyboard capture
    void ReleaseKeyboardCapture(const Ptr<Input::InputHandler>& inputHandler);

    /// return true if the application has focus
    bool HasFocus() const;

protected:
    bool isOpen;
    bool inBeginFrame;
    bool isQuitRequested;
    bool hasFocus;
    int inputHandlersLockCount;
    SizeT maxNumLocalPlayers;
    Util::Array<Util::KeyValuePair<Input::InputPriority::Code,Ptr<Input::InputHandler> > > inputHandlers;
    Ptr<Input::InputHandler> mouseCaptureHandler;
    Ptr<Input::InputHandler> keyboardCaptureHandler;
    Ptr<Input::Keyboard> defaultKeyboard;
    Ptr<Input::Mouse> defaultMouse;
    Util::FixedArray<Ptr<Input::GamePad> > defaultGamePad; 
    Threading::CriticalSection critSect;    
    Threading::ThreadId creatorThreadId;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
InputServerBase::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
InputServerBase::GetMaxNumLocalPlayers() const
{
    return this->maxNumLocalPlayers;
}

//------------------------------------------------------------------------------
/**
*/
inline void
InputServerBase::SetQuitRequested(bool b)
{
    this->isQuitRequested = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
InputServerBase::IsQuitRequested() const
{
    return this->isQuitRequested;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Input::Keyboard>&
InputServerBase::GetDefaultKeyboard() const
{    
    n_assert2(creatorThreadId == Threading::Thread::GetMyThreadId(), "GetDefaultKeyboard can't be called from any thread but the creator thread!");
    return this->defaultKeyboard;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Input::Mouse>&
InputServerBase::GetDefaultMouse() const
{
    n_assert2(creatorThreadId == Threading::Thread::GetMyThreadId(), "GetDefaultMouse can't be called from any thread but the creator thread!");
    return this->defaultMouse;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
InputServerBase::HasFocus() const
{
    return hasFocus;
}

} // namespace Base
//------------------------------------------------------------------------------

    
    