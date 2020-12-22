#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::GamePadBase

    An input handler which represents one of at most 4 game pads.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "input/inputhandler.h"
#include "threading/criticalsection.h"
#include "threading/thread.h"

#define MAX_GAMEPADS 4

//------------------------------------------------------------------------------
namespace Base
{
class GamePadBase : public Input::InputHandler
{
    __DeclareClass(GamePadBase);
public:
    /// gamepad buttons
    enum Button
    {
        DPadUpButton = 0,           // all platforms
        DPadDownButton,             // all platforms        
        DPadLeftButton,             // all platforms
        DPadRightButton,            // all platforms
        StartButton,                // all platforms
        BackButton,                 // Xbox360
        LeftThumbButton,            // Xbox360, PS3: L3
        RightThumbButton,           // Xbox360, PS3: R3
        LeftShoulderButton,         // Xbox360, PS3: L1, Wii: ClassicController ZL
        RightShoulderButton,        // Xbox360, PS3: R1, Wii: ClassicController ZR
        
        AButton,                    // Xbox360: A, Wii: A/a, PS3: Cross
        BButton,                    // Xbox360: B, Wii: b(pad), PS3: Circle
        XButton,                    // Xbox360: X, Wii: x(pad), PS3: Square
        YButton,                    // Xbox360: Y, Wii: y(pad), PS3: Triangle
        
        // Wii specific buttons
        HomeButton,                 // Xbox360: /, Wii: Home, PS3: /
        PlusButton,                 // Xbox360: /, Wii: +, PS3: /
        MinusButton,                // Xbox360: /, Wii: -, PS3: /
        OneButton,                  // Xbox360: /, Wii: 1, PS3: /
        TwoButton,                  // Xbox360: /, Wii: 2, PS3: /
        ZButton,                    // Xbox360: /, Wii: Z(Nunchuk), PS3: /
        CButton,                    // Xbox360: /, Wii: C(Nunchuk), PS3: /

        // PS3 specific buttons
        SelectButton,               // Xbox360: /, Wii: /, PS3: Select

        NumButtons,
        InvalidButton,


        // PS3 aliases !!! MUST BE after NumButtons & InvalidButton !!!
        CrossButton = AButton,
        CircleButton = BButton,
        SquareButton = XButton,
        TriangleButton = YButton,
        L1Button = LeftShoulderButton,
        R1Button = RightShoulderButton,
        L3Button = LeftThumbButton,
        R3Button = RightThumbButton,
    };

    /// gamepad axis
    enum Axis
    {
        // general axi
        LeftTriggerAxis = 0,    // 0.0f .. 1.0f
        RightTriggerAxis,       // 0.0f .. 1.0f
        LeftThumbXAxis,         // -1.0f .. 1.0f
        LeftThumbYAxis,         // -1.0f .. 1.0f
        RightThumbXAxis,        // -1.0f .. 1.0f
        RightThumbYAxis,        // -1.0f .. 1.0f
        
        NumAxes,
        InvalidAxis,
    };

    /// constructor
    GamePadBase();
    /// destructor
    virtual ~GamePadBase();
    
    /// convert button code to string
    static Util::String ButtonAsString(Button btn);
    /// convert axis to string
    static Util::String AxisAsString(Axis a);

    /// return true if this game pad is currently connected
    bool IsConnected() const;
    /// set index -> TODO make threadsafe
    void SetIndex(IndexT i);
    /// get the index of this game pad
    IndexT GetIndex() const;
    /// get maximum number of controllers
    static SizeT GetMaxNumControllers();

    /// return true if a button is currently pressed
    bool ButtonPressed(Button btn) const;
    /// return true if button was down at least once in current frame
    bool ButtonDown(Button btn) const;
    /// return true if button was up at least once in current frame
    bool ButtonUp(Button btn) const;
    /// get current axis value
    float GetAxisValue(Axis axis) const;
    /// check if device has 3d location sensor
    bool HasTransform() const;
    /// get current position (if supported by device, identity otherwise)
    const Math::mat4 & GetTransform() const;

    /// set low-frequency vibration effect (0.0f .. 1.0f)
    void SetLowFrequencyVibrator(float f);
    /// get low-frequency vibration
    float GetLowFrequencyVibrator() const;
    /// set high-frequency vibration effect (0.0f .. 1.0f)
    void SetHighFrequencyVibrator(float f);
    /// get high-frequency vibration
    float GetHighFrequencyVibrator() const;

    /// get current state as an array of input events (override in subclass!)
    Util::Array<Input::InputEvent> GetStateAsInputEvents() const;

protected:   
    /// called when the handler is attached to the input server
    virtual void OnAttach();
    /// reset the input handler
    virtual void OnReset();

    struct ButtonState
    {
    public:
        /// constructor
        ButtonState() : pressed(false), down(false), up(false) {};

        bool pressed;
        bool down;
        bool up;
    };

    IndexT index;
    bool isConnected;
    Util::FixedArray<ButtonState> buttonStates;
    Util::FixedArray<float> axisValues;
    bool vibratorsDirty;
    bool hasTransform;
    float lowFreqVibrator;
    float highFreqVibrator;
    Threading::CriticalSection critSect;
    Threading::ThreadId creatorThreadId;
    Math::mat4 transform;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
GamePadBase::IsConnected() const
{
    n_assert2(creatorThreadId == Threading::Thread::GetMyThreadId(), "IsConnected can't be called from any thread but the creator thread!");
    return this->isConnected;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
GamePadBase::HasTransform() const
{   
    return this->hasTransform;
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Math::mat4 &
GamePadBase::GetTransform() const
{   
    return this->transform;
}

//------------------------------------------------------------------------------
/**
*/
inline void
GamePadBase::SetIndex(IndexT i)
{
    n_assert(i < this->GetMaxNumControllers());
    this->index = i;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
GamePadBase::GetIndex() const
{
    return this->index;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
GamePadBase::GetMaxNumControllers()
{
    return MAX_GAMEPADS;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
GamePadBase::ButtonPressed(Button btn) const
{
    n_assert2(creatorThreadId == Threading::Thread::GetMyThreadId(), "ButtonPressed can't be called from any thread but the creator thread!");
    return this->buttonStates[btn].pressed;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
GamePadBase::ButtonDown(Button btn) const
{
    n_assert2(creatorThreadId == Threading::Thread::GetMyThreadId(), "ButtonDown can't be called from any thread but the creator thread!");
    return this->buttonStates[btn].down;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
GamePadBase::ButtonUp(Button btn) const
{
    n_assert2(creatorThreadId == Threading::Thread::GetMyThreadId(), "ButtonUp can't be called from any thread but the creator thread!");
    return this->buttonStates[btn].up;
}

//------------------------------------------------------------------------------
/**
*/
inline float
GamePadBase::GetAxisValue(Axis axis) const
{
    n_assert2(creatorThreadId == Threading::Thread::GetMyThreadId(), "GetAxisValue can't be called from any thread but the creator thread!");
    return this->axisValues[axis];
}

//------------------------------------------------------------------------------
/**
*/
inline void
GamePadBase::SetLowFrequencyVibrator(float f)
{        
    critSect.Enter();
    this->vibratorsDirty = true;
    this->lowFreqVibrator = Math::n_clamp(f, 0.0f, 1.0f);
    critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
inline float
GamePadBase::GetLowFrequencyVibrator() const
{                    
    critSect.Enter();
    float result = this->lowFreqVibrator;
    critSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
inline void
GamePadBase::SetHighFrequencyVibrator(float f)
{   
    critSect.Enter();
    this->vibratorsDirty = true;
    this->highFreqVibrator = Math::n_clamp(f, 0.0f, 1.0f);;
    critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
inline float
GamePadBase::GetHighFrequencyVibrator() const
{   
    critSect.Enter();
    float result = this->highFreqVibrator;
    critSect.Leave();
    return result;
}

} // namespace Base
//------------------------------------------------------------------------------

