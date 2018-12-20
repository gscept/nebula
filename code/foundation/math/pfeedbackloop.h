#ifndef Math_PFEEDBACKLOOP_H
#define Math_PFEEDBACKLOOP_H
//------------------------------------------------------------------------------
/**
    @class PFeedbackLoop
    @ingroup Math

    A P feedback loop (proportional feedback loop) is a simple object which 
    moves a system's current state towards a goal, using the resulting error 
    (difference between goal and state as feedback on the next run.

    If you need to implement motion controllers, camera controllers, etc...
    then the feedback loop is your friend.

    See Game Developer Mag issue June/July 2004.

    (C) 2007 RadonLabs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "timing/time.h"

namespace Math
{
//------------------------------------------------------------------------------
template<class TYPE> class PFeedbackLoop
{
public:
    /// constructor
    PFeedbackLoop();
    /// reset the time
    void Reset(Timing::Time time, float stepSize, float gain, const TYPE& curState);
    /// set the gain
    void SetGain(float g);
    /// get the gain
    float GetGain() const;
    /// set the goal
    void SetGoal(const TYPE& c);
    /// get the goal
    const TYPE& GetGoal() const;
    /// set the current state directly
    void SetState(const TYPE& s);
    /// get the current state the system is in
    const TYPE& GetState() const;
    /// update the object, return new state
    void Update(Timing::Time time);

private:
    Timing::Time time;         // the time at which the simulation is
    float stepSize;
    float gain;
    TYPE goal;
    TYPE state;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
PFeedbackLoop<TYPE>::PFeedbackLoop() :
    time(0.0),
    stepSize(0.001f),
    gain(-1.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
PFeedbackLoop<TYPE>::Reset(Timing::Time t, float s, float g, const TYPE& curState)
{
    this->time = t;
    this->stepSize = s;
    this->gain = g;
    this->state = curState;
    this->goal  = curState;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
PFeedbackLoop<TYPE>::SetGain(float g)
{
    this->gain = g;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
float
PFeedbackLoop<TYPE>::GetGain() const
{
    return this->gain;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
PFeedbackLoop<TYPE>::SetGoal(const TYPE& g)
{
    this->goal = g;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
const TYPE&
PFeedbackLoop<TYPE>::GetGoal() const
{
    return this->goal;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
PFeedbackLoop<TYPE>::SetState(const TYPE& s)
{
    this->state = s;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
const TYPE&
PFeedbackLoop<TYPE>::GetState() const
{
    return this->state;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
PFeedbackLoop<TYPE>::Update(Timing::Time curTime)
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
        this->state = this->state + (this->state - this->goal) * this->gain * this->stepSize;
        this->time += this->stepSize;
    }
}
} // namespace Math
//------------------------------------------------------------------------------
#endif