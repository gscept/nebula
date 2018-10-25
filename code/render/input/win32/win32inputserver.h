#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32InputServer
    
    Win32-specific InputServer (provides a default Keyboard and Mouse).
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/singleton.h"
#include "input/base/inputserverbase.h"
#include "input/win32/win32inputdisplayeventhandler.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32InputServer : public Base::InputServerBase
{
    __DeclareClass(Win32InputServer);
    __DeclareSingleton(Win32InputServer);
public:
    /// constructor
    Win32InputServer();
    /// destructor
    virtual ~Win32InputServer();

    /// open the input server
    void Open();
    /// close the input server
    void Close();
    /// call after processing window events (reads DInput raw mouse events)
    void OnFrame();

protected:
    friend class Win32Mouse;

    /// setup the DirectInput mouse device for tracking mouse movement 
    bool OpenDInputMouse();
    /// shutdown the DirectInput mouse device
    void CloseDInputMouse();
    /// get mouse readings
    void ReadDInputMouse();
    /// get the current mouse movement
    const Math::float2& GetMouseMovement() const;
	/// set the mouse to a screen space position
	void SetMousePosition(Math::float2& position);

    Ptr<Win32InputDisplayEventHandler> eventHandler;
    IDirectInput8* di8;
    IDirectInputDevice8* di8Mouse;
	POINT previousPos;
    Math::float2 mouseMovement;
    static const int DInputMouseBufferSize = 16;
};

//------------------------------------------------------------------------------
/**
*/
inline const Math::float2&
Win32InputServer::GetMouseMovement() const
{
    return this->mouseMovement;
}

} // namespace Win32
//------------------------------------------------------------------------------