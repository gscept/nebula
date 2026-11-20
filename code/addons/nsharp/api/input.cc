//------------------------------------------------------------------------------
//  @file nsharp/api/input.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "input.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/key.h"
#include "input/mousebutton.h"

namespace Scripting
{
namespace Api
{

//------------------------------------------------------------------------------
/**
    The state is read as it stands when the call is made, which is why the managed side calls this at the
    beginning of its frame and not anywhere else. The per-frame flags are cleared at the start of the input
    frame and set again by the events that arrive during the pump, so a sync before the pump reads the frame
    that is about to be built, and a sync after EndFrame() reads nothing.
*/
int
InputSync(void* buffer, int bufferSize)
{
    n_assert2(
        bufferSize == (int)sizeof(InputSnapshot),
        "InputSync: Provided buffer size is not the size of the snapshot layout."
    );

    InputSnapshot* out = (InputSnapshot*)buffer;
    Memory::Clear(out, sizeof(InputSnapshot));

    if (!Input::InputServer::HasInstance())
        return 0;

    Input::InputServer* server = Input::InputServer::Instance();

    Ptr<Input::Keyboard> keyboard = server->GetDefaultKeyboard();
    if (keyboard.isvalid())
    {
        for (IndexT i = 0; i < Input::Key::NumKeyCodes; i++)
        {
            int word = (int)i / 64;
            uint64_t bit = 1ULL << ((int)i % 64);

            if (keyboard->KeyPressed((Input::Key::Code)i))
                out->keys[word].held |= bit;

            if (keyboard->KeyDown((Input::Key::Code)i))
                out->keys[word].down |= bit;

            if (keyboard->KeyUp((Input::Key::Code)i))
                out->keys[word].up |= bit;
        }
    }

    Ptr<Input::Mouse> mouse = server->GetDefaultMouse();
    if (mouse.isvalid())
    {
        for (IndexT i = 0; i < Input::MouseButton::NumMouseButtons; i++)
        {
            uint32_t bit = 1u << (int)i;

            if (mouse->ButtonPressed((Input::MouseButton::Code)i))
                out->mouseHeld |= bit;

            if (mouse->ButtonDown((Input::MouseButton::Code)i))
                out->mouseDown |= bit;

            if (mouse->ButtonUp((Input::MouseButton::Code)i))
                out->mouseUp |= bit;

            if (mouse->ButtonDoubleClicked((Input::MouseButton::Code)i))
                out->mouseDoubleClicked |= bit;
        }

        // The handlers only have a forward and a backward flag, so the wheel comes across as the net
        // movement for the frame rather than as two booleans
        out->wheel = (mouse->WheelForward() ? 1 : 0) - (mouse->WheelBackward() ? 1 : 0);

        out->pixelX = mouse->GetPixelPosition().x;
        out->pixelY = mouse->GetPixelPosition().y;
        out->screenX = mouse->GetScreenPosition().x;
        out->screenY = mouse->GetScreenPosition().y;
        out->moveX = mouse->GetMovement().x;
        out->moveY = mouse->GetMovement().y;
    }

    out->hasFocus = server->HasFocus() ? 1 : 0;
    out->mouseCaptured = server->GetMouseCaptureHandler().isvalid() ? 1 : 0;
    out->keyCaptured = server->GetKeyboardCaptureHandler().isvalid() ? 1 : 0;

    return (int)sizeof(InputSnapshot);
}

//------------------------------------------------------------------------------
/**
*/
int
InputCharInput(char* outBuffer, int outLen)
{
    if (outBuffer == nullptr || outLen <= 1)
        return 0;

    if (!Input::InputServer::HasInstance())
        return 0;

    Ptr<Input::Keyboard> keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();
    if (!keyboard.isvalid())
        return 0;

    const Util::String& input = keyboard->GetCharInput();

    int written = (int)input.Length();
    if (written > outLen - 1)
        written = outLen - 1;

    Memory::Copy(input.AsCharPtr(), outBuffer, written);
    outBuffer[written] = 0;

    return written;
}

} // namespace Api
} // namespace Scripting
