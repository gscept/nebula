#pragma once
//------------------------------------------------------------------------------
/**
	@file nsharp/api/input.h

	The Input::InputServer as a C boundary for the managed runtime.

	The input server is event driven: events come in through the window and travel the handler chain, and the
	default keyboard and mouse handlers are the handlers that present what they received as a state an
	application can poll. This is that polling state, copied into a buffer C# owns.

	The copy is only this frame's between the point where the events have been pumped and InputServer's
	EndFrame(), which is what clears the per-frame flags. In the game application the render feature unit
	pumps the window events at the start of its OnBeginFrame, and the script feature unit is attached after it,
	so the managed sync at the beginning of the managed frame is the point where the state is the frame's own.

	What the state holds is what the input server received. ImGui gets the keys first and the window drops the
	events it consumes, so a key typed into a text field never reaches the handlers. That is what the focus and
	capture flags are for: they tell a caller that a key is missing because the UI took it rather than because
	nobody pressed it.

	@copyright
	(C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "nsharp/nsconfig.h"

namespace Scripting
{
namespace Api
{

//------------------------------------------------------------------------------
/**
	One key as three words: held through the frame, pressed during it, released during it. Three words cover
	the 135 key codes, and a code decides its word and its bit, so the managed side reads a key without
	looking anything up.
*/
struct KeyWords
{
    uint64_t held;
    uint64_t down;
    uint64_t up;
};

//------------------------------------------------------------------------------
/**
	The layout the managed input mirror is built from. The eight byte fields come first and then the four byte
	ones, which is the order C# declares as well. In this order the entry is 128 bytes on both sides with no
	padding, and the field order is what makes that true.
*/
struct InputSnapshot
{
    KeyWords keys[3];

    uint32_t mouseHeld;
    uint32_t mouseDown;
    uint32_t mouseUp;
    uint32_t mouseDoubleClicked;
    int32_t wheel;

    uint32_t hasFocus;
    uint32_t mouseCaptured;
    uint32_t keyCaptured;

    float pixelX;
    float pixelY;
    float screenX;
    float screenY;
    float moveX;
    float moveY;
};

static_assert(sizeof(InputSnapshot) == 128, "managed input mirror layout");

//------------------------------------------------------------------------------
/**
	Copy the state of the default keyboard and mouse handlers into the buffer C# owns. Returns the number of
	bytes written, or 0 when there is no input server, which is what a build with no window has. The managed
	side clears its mirror when nothing is written, so a windowless run reads nothing rather than the state the
	previous frame had.

	This has to be in the object library for a managed DllImport to find it, same as the other entry points.
*/
NEBULA_EXPORT int InputSync(void* buffer, int bufferSize);

//------------------------------------------------------------------------------
/**
	The text the keyboard produced during this frame, as utf8 into the buffer C# owns. Text cannot come back
	through the marshaler, because a DllImport that returns a string makes the marshaler free the pointer it
	was given, so this is the only way text crosses. The buffer is terminated, and the answer is the number of
	bytes written, not counting the terminator.
*/
NEBULA_EXPORT int InputCharInput(char* outBuffer, int outLen);

} // namespace Api
} // namespace Scripting
