//------------------------------------------------------------------------------
//  win32inputserver.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "input/win32/win32inputserver.h"
#include "coregraphics/displaydevice.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/gamepad.h"
#include "graphics/display.h"

namespace Win32
{
__ImplementClass(Win32::Win32InputServer, 'W3IS', Base::InputServerBase);
__ImplementSingleton(Win32::Win32InputServer);

using namespace Input;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
Win32InputServer::Win32InputServer() :
    di8(0),
    di8Mouse(0)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
Win32InputServer::~Win32InputServer()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**    
*/
void
Win32InputServer::Open()
{
    n_assert(!this->IsOpen());
    InputServerBase::Open();

    // initialize DirectInput for raw mouse events
    this->OpenDInputMouse();

    // setup a display event handler which translates
    // some display events into input events
    this->eventHandler = Win32InputDisplayEventHandler::Create();
    Graphics::Display::Instance()->AttachDisplayEventHandler(this->eventHandler.upcast<ThreadSafeDisplayEventHandler>());

    // create a default keyboard and mouse handler
    this->defaultKeyboard = Keyboard::Create();
    this->AttachInputHandler(InputPriority::Game, this->defaultKeyboard.upcast<InputHandler>());
    this->defaultMouse = Mouse::Create();
    this->AttachInputHandler(InputPriority::Game, this->defaultMouse.upcast<InputHandler>());

    // create 4 default gamepads (none of them have to be connected)
    IndexT playerIndex = MAX_GAMEPADS;
    for (playerIndex = 0; playerIndex < this->maxNumLocalPlayers; playerIndex++)
    {
        this->defaultGamePad[playerIndex] = GamePad::Create();
        this->defaultGamePad[playerIndex]->SetIndex(playerIndex);
        this->AttachInputHandler(InputPriority::Game, this->defaultGamePad[playerIndex].upcast<InputHandler>());
    }
}

//------------------------------------------------------------------------------
/**    
*/
void
Win32InputServer::Close()
{
    n_assert(this->IsOpen());

    // remove our event handler from the display device
    Graphics::Display::Instance()->RemoveDisplayEventHandler(this->eventHandler.upcast<ThreadSafeDisplayEventHandler>());

    // shutdown the DirectInput mouse device
    this->CloseDInputMouse();

    // call parent class
    InputServerBase::Close();
}

//------------------------------------------------------------------------------
/**    
*/
void
Win32InputServer::OnFrame()
{
    this->eventHandler->HandlePendingEvents();
    this->ReadDInputMouse();
    InputServerBase::OnFrame();
}

//------------------------------------------------------------------------------
/**    
    This intitialies a DirectInput mouse device in order to track
    raw mouse movement (WM mouse events stop at the screen borders).
*/
bool
Win32InputServer::OpenDInputMouse()
{
    n_assert(0 == this->di8);
    n_assert(0 == this->di8Mouse);
    HRESULT hr;

    // create DirectInput interface
    hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&(this->di8), NULL);
    n_assert(SUCCEEDED(hr));
    n_assert(0 != this->di8);

    // create a DirectInput mouse device
    hr = this->di8->CreateDevice(GUID_SysMouse, &(this->di8Mouse), NULL);
    n_assert(SUCCEEDED(hr));
    n_assert(0 != this->di8Mouse);

    // tell DInput what we're interested in
    hr = this->di8Mouse->SetDataFormat(&c_dfDIMouse2);
    n_assert(SUCCEEDED(hr));

    // set the cooperative level of the device, we're friendly
    // note: use Win32's FindWindow() to find our top level window because 
    // the DisplayDevice may be running in a different thread
    HWND hWnd = FindWindow(NEBULA3_WINDOW_CLASS, NULL);
    if (0 != hWnd)
    {
        hr = this->di8Mouse->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NOWINKEY | DISCL_NONEXCLUSIVE);
        n_assert(SUCCEEDED(hr));
    }

    // set buffer size and relative axis mode on the mouse
    DIPROPDWORD dipdw;
    dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj        = 0;
    dipdw.diph.dwHow        = DIPH_DEVICE;

    dipdw.dwData = DInputMouseBufferSize;
    hr = this->di8Mouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);
    n_assert(SUCCEEDED(hr));
    dipdw.dwData = DIPROPAXISMODE_REL;
    hr = this->di8Mouse->SetProperty(DIPROP_AXISMODE, &dipdw.diph);
    n_assert(SUCCEEDED(hr));

    // aquire the mouse
    this->di8Mouse->Acquire();

    return true;
}

//------------------------------------------------------------------------------
/**    
    Close the DirectInput mouse and DirectInput.
*/
void
Win32InputServer::CloseDInputMouse()
{
    n_assert(0 != this->di8);
    n_assert(0 != this->di8Mouse);
    this->di8Mouse->Unacquire();
    this->di8Mouse->Release();
    this->di8Mouse = 0;
    this->di8->Release();
    this->di8 = 0;
}

//------------------------------------------------------------------------------
/**    
    Read data from the DirectInput mouse (relative mouse movement
    since the last frame).
*/
void
Win32InputServer::ReadDInputMouse()
{
    n_assert(0 != this->di8Mouse);
    //DIDEVICEOBJECTDATA didod[DInputMouseBufferSize];
    //HRESULT hr;
    
    this->mouseMovement.set(0.0f, 0.0f);
	POINT currentPos;
	GetCursorPos(&currentPos);

	this->mouseMovement.x() = (Math::scalar)(this->previousPos.x - currentPos.x);
	this->mouseMovement.y() = (Math::scalar)(this->previousPos.y - currentPos.y);

	if (this->defaultMouse->IsLocked())
		SetCursorPos(previousPos.x, previousPos.y);
	else
		this->previousPos = currentPos;

	/*
    // read buffered mouse data
    DWORD inOutNumElements = DInputMouseBufferSize;
    hr = this->di8Mouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), didod, &inOutNumElements, 0);
    if (DI_OK != hr)
    {
        hr = this->di8Mouse->Acquire();
    }
    else
    {
        IndexT i;
        for (i = 0; i < (SizeT)inOutNumElements; i++)
        {
            switch (didod[i].dwOfs)
            {
                case DIMOFS_X:
                    this->mouseMovement.x() += float(int(didod[i].dwData));
                    break;

                case DIMOFS_Y:
                    this->mouseMovement.y() += float(int(didod[i].dwData));
                    break;
            }
        }
    }
	*/
}

//------------------------------------------------------------------------------
/**
*/
void
Win32InputServer::SetMousePosition(Math::float2& position)
{
	WINDOWINFO windowInfo;
	HWND nebWindow = FindWindow(NEBULA3_WINDOW_CLASS, NULL);
	
	if (nebWindow != 0 && GetWindowInfo(nebWindow, &windowInfo))
	{
		this->previousPos.x = windowInfo.rcWindow.left + (uint)( (windowInfo.rcClient.right - windowInfo.rcClient.left) * position.x() + 0.5f );
		this->previousPos.y = windowInfo.rcWindow.top + (uint)( (windowInfo.rcClient.bottom - windowInfo.rcClient.top) * position.y() + 0.5f );

		SetCursorPos(previousPos.x, previousPos.y);
	}
}

} // namespace Win32