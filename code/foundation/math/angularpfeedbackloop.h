#ifndef MATH_ANGULARPFEEDBACKLOOP_H
#define MATH_ANGULARPFEEDBACKLOOP_H
//------------------------------------------------------------------------------
/** 
    @class AngularPFeedbackLoop
    @ingroup Util

    A proportional feedback loop with correct angular interpolation.
    
    (C) 2004 RadonLabs GmbH
	(C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "timing/time.h"

namespace Math
{

//------------------------------------------------------------------------------
class AngularPFeedbackLoop
{
public:
    /// constructor
    AngularPFeedbackLoop();
    /// reset the time
    void Reset(Timing::Time time, float stepSize, float gain, float curState);
    /// set the gain
    void SetGain(float g);
    /// get the gain
    float GetGain() const;
    /// set the goal
    void SetGoal(float c);
    /// get the goal
    float GetGoal() const;
    /// set the current state directly
    void SetState(float s);
    /// get the current state the system is in
    float GetState() const;
    /// update the object, return new state
    void Update(Timing::Time time);
    /// get currrent time
    Timing::Time GetTime() const;

private:
    Timing::Time time;         // the time at which the simulation is
    float stepSize;
    float gain;
    float goal;
    float state;
};

//------------------------------------------------------------------------------
/**
*/
inline
AngularPFeedbackLoop::AngularPFeedbackLoop() :
    time(0.0),
    stepSize(0.001f),
    gain(-1.0f),
    goal(0.0f),
    state(0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
AngularPFeedbackLoop::Reset(Timing::Time t, float s, float g, float curState)
{
    this->time = t;
    this->stepSize = s;
    this->gain = g;
    this->state = curState;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
AngularPFeedbackLoop::SetGain(float g)
{
    this->gain = g;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
AngularPFeedbackLoop::GetGain() const
{
    return this->gain;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
AngularPFeedbackLoop::SetGoal(float g)
{
    this->goal = g;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
AngularPFeedbackLoop::GetGoal() const
{
    return this->goal;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
AngularPFeedbackLoop::SetState(float s)
{
    this->state = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
AngularPFeedbackLoop::GetState() const
{
    return this->state;
}

//------------------------------------------------------------------------------
/**
*/
inline
Timing::Time 
AngularPFeedbackLoop::GetTime() const
{
    return this->time;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
AngularPFeedbackLoop::Update(Timing::Time curTime)
{
    Timing::Time dt = curTime - this->time;

    // catch time exceptions
    if (dt < 0.0) 
    {
        this->time = curTime;
    }
    else if (dt > 0.5)
    {
        this->time = curTime - 0.5;
    }

    while (this->time < curTime)
    {
        // get angular distance error
        float error = n_angulardistance(this->state, this->goal);
        if (n_abs(error) > N_TINY)
        {
            this->state = n_modangle(this->state - (error * this->gain * this->stepSize));
            this->time += this->stepSize;
        }
        else
        {
            this->state = this->goal;
            this->time = curTime;
            break;
        }
    }
}

} // namespace Math
//------------------------------------------------------------------------------
#endif