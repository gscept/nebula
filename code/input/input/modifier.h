#pragma once
//------------------------------------------------------------------------------
/**
    @namespace Input::Modifier

    The modifier keys as a bitmask.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "input/key.h"
#include "util/string.h"

namespace Input
{
namespace Modifier
{

using Code = uint32_t;

constexpr Code NoModifier = 0;

constexpr Code LeftControlModifier  = 1 << 0;
constexpr Code RightControlModifier = 1 << 1;
constexpr Code LeftShiftModifier    = 1 << 2;
constexpr Code RightShiftModifier   = 1 << 3;
constexpr Code LeftAltModifier      = 1 << 4;
constexpr Code RightAltModifier     = 1 << 5;
constexpr Code LeftSuperModifier    = 1 << 6;
constexpr Code RightSuperModifier   = 1 << 7;

constexpr Code ControlModifier = LeftControlModifier | RightControlModifier;
constexpr Code ShiftModifier   = LeftShiftModifier | RightShiftModifier;
constexpr Code AltModifier     = LeftAltModifier | RightAltModifier;
constexpr Code SuperModifier   = LeftSuperModifier | RightSuperModifier;

constexpr Code GroupMasks[] = { ControlModifier, ShiftModifier, AltModifier, SuperModifier };
constexpr const char* GroupNames[] = { "ctrl", "shift", "alt", "super" };
constexpr IndexT NumGroups = 4;

constexpr Key::Code Keys[] = {
    Key::LeftControl, Key::RightControl,
    Key::LeftShift, Key::RightShift,
    Key::LeftMenu, Key::RightMenu,
    Key::LeftWindows, Key::RightWindows,
    Key::Control, Key::Shift, Key::Menu,
};
constexpr IndexT NumKeys = 11;

//------------------------------------------------------------------------------
/**
    The bit a key code has, or NoModifier when the key is not a modifier key
*/
inline Code
FromKey(Key::Code key)
{
    switch (key)
    {
    case Key::LeftControl:  return LeftControlModifier;
    case Key::RightControl: return RightControlModifier;
    case Key::LeftShift:    return LeftShiftModifier;
    case Key::RightShift:   return RightShiftModifier;
    case Key::LeftMenu:     return LeftAltModifier;
    case Key::RightMenu:    return RightAltModifier;
    case Key::LeftWindows:  return LeftSuperModifier;
    case Key::RightWindows: return RightSuperModifier;

    case Key::Control:      return ControlModifier;
    case Key::Shift:        return ShiftModifier;
    case Key::Menu:         return AltModifier;

    default:
        return NoModifier;
    }
}

//------------------------------------------------------------------------------
/**
    Returns true when the key code is one of the modifier keys.
*/
inline bool
IsModifier(Key::Code key)
{
    return FromKey(key) != NoModifier;
}

//------------------------------------------------------------------------------
/**
    The group a bit belongs to. A group bit answers with itself.
*/
inline Code
Logical(Code bit)
{
    for (IndexT i = 0; i < NumGroups; i++)
    {
        if (bit & GroupMasks[i])
            return GroupMasks[i];
    }
    return NoModifier;
}

//------------------------------------------------------------------------------
/**
    Return true when the held bits satisfy the requirement
*/
inline bool
Matches(Code required, Code held)
{
    for (IndexT i = 0; i < NumGroups; i++)
    {
        const Code want = required & GroupMasks[i];
        if (want == 0)
            continue;

        if (want == GroupMasks[i])
        {
            if ((held & GroupMasks[i]) == 0)
                return false;
        }
        else if ((held & want) != want)
            return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    The number of groups a requirement has
*/
inline IndexT
Count(Code required)
{
    IndexT count = 0;
    for (IndexT i = 0; i < NumGroups; i++)
    {
        if (required & GroupMasks[i])
            count++;
    }
    return count;
}

/// convert the modifiers to the combo string, as the groups in the order a combo is written in
Util::String ToString(Code mods);
/// read one combo token as a modifier. Returns NoModifier when the token is not a modifier
Code FromString(const Util::String& token);

} // namespace Modifier
} // namespace Input
//------------------------------------------------------------------------------
