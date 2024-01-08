//------------------------------------------------------------------------------
//  gamepadbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "input/base/gamepadbase.h"

namespace Base
{
__ImplementClass(Base::GamePadBase, 'IGPB', Input::InputHandler);

using namespace Input;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
GamePadBase::GamePadBase() :
    index(0),
    isConnected(false),
    buttonStates(NumButtons),
    axisValues(NumAxes, 0.0f),
    vibratorsDirty(false),
    hasTransform(false),
    lowFreqVibrator(0.0f),
    highFreqVibrator(0.0f),
    creatorThreadId(Threading::Thread::GetMyThreadId())
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
GamePadBase::~GamePadBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
GamePadBase::OnAttach()
{
    InputHandler::OnAttach();

    // need to reset our state
    ButtonState initialButtonState;
    this->buttonStates.Fill(initialButtonState);
    this->axisValues.Fill(0.0f);
}

//------------------------------------------------------------------------------
/**
*/
void
GamePadBase::OnReset()
{
    InputHandler::OnReset();

    // reset button states (should behave as if the player
    // suddenly released all buttons and triggers)
    IndexT i;
    for (i = 0; i < this->buttonStates.Size(); i++)
    {
        ButtonState& btnState = this->buttonStates[i];
        if (btnState.pressed)
        {
            btnState.up = true;
        }
        else
        {
            btnState.up = false;
        }
        btnState.down = false;
        btnState.pressed = false;
    }
    this->axisValues.Fill(0.0f);
}

//------------------------------------------------------------------------------
/**
*/
String
GamePadBase::ButtonAsString(Button btn)
{
    switch (btn)
    {
        case DPadUpButton:          return "DPadUpButton";
        case DPadDownButton:        return "DPadDownButton";
        case DPadLeftButton:        return "DPadLeftButton";
        case DPadRightButton:       return "DPadRightButton";
        case StartButton:           return "StartButton";
        case BackButton:            return "BackButton";
        case LeftThumbButton:       return "LeftThumbButton";
        case RightThumbButton:      return "RightThumbButton";
        case LeftShoulderButton:    return "LeftShoulderButton";        
        case RightShoulderButton:   return "RightShoulderButton";        
        case AButton:               return "AButton";
        case BButton:               return "BButton";
        case XButton:               return "XButton";
        case YButton:               return "YButton";
        case HomeButton:            return "HomeButton";
        case PlusButton:            return "PlusButton";
        case MinusButton:           return "MinusButton";
        case OneButton:             return "OneButton";
        case TwoButton:             return "TwoButton";
        case ZButton:               return "ZButton";
        case CButton:               return "CButton";
        case SelectButton:          return "SelectButton";
        default:
            n_error("GamePadBase::ButtonAsString(): invalid button code!\n");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
*/
String
GamePadBase::AxisAsString(Axis axis)
{
    switch (axis)
    {
        case LeftTriggerAxis:   return "LeftTriggerAxis";
        case RightTriggerAxis:  return "RightTriggerAxis";
        case LeftThumbXAxis:    return "LeftThumbXAxis";
        case LeftThumbYAxis:    return "LeftThumbYAxis";
        case RightThumbXAxis:   return "RightThumbXAxis";
        case RightThumbYAxis:   return "RightThumbYAxis";
        default:
            n_error("GamePadBase::AxisAsString(): invalid axis code!\n");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
    This method should return the current state of the game pad
    as input events. It is up to a specific subclass to implement
    this method.
*/
Array<InputEvent>
GamePadBase::GetStateAsInputEvents() const
{
    Array<InputEvent> emptyArray;
    return emptyArray;
}

} // namespace Base
