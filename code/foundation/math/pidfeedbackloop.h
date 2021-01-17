#ifndef Math_PIDFEEDBACKLOOP_H
#define Math_PIDFEEDBACKLOOP_H
//------------------------------------------------------------------------------
/**
    @class Math::PIDFeedbackLoop

    A PID feedback loop (proportional integral derivative feedback loop) 

    @copyright
    (C) 2007 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "timing/time.h"

namespace Math
{
class PIDFeedbackLoop
{
public:
    /// constructor
    PIDFeedbackLoop();
    /// set value of loop
    void SetState(double value);
    /// set the goal
    void SetGoal(double wantedValue);
    /// set the propotional, integral and derivative constants, and maximum acceleration (how fast the value kann change, will be disabled if set to 0.0 (default))
    void SetConstants(double pConst, double iConst, double dConst, double acceleration = 0.0);
    /// get current value
    const double& GetState() const;
    /// get the goal
    const double& GetGoal() const;
    /// get last computed error
    double GetLastError() const;
    /// last delta of error
    double GetLastDelta() const;
    /// update current value
    void Update(Timing::Time time); 
    /// reset running error
    void ResetError();
    /// set IsAngularValue
    void SetIsAngularValue(bool val);

private:
    double value;               // current value of the controller 
    double goal;            // the value the controller is trying to achieve 
           
    double pConst;              // proportional constant (Kp) 
    double iConst;              // integral constant (Ki) 
    double dConst;              // derivative constant (Kd) 
    double maxAcceleration;     // limits how fast the control can accelerate the value 
           
    double lastError;           // previous error 
    double lastDelta;           // amount of change during last adjustment 
    double runningError;        // summed errors (using as the integral value) 
    bool validError;            // prevents numerical problems on the first adjustment  
    bool isAngularValue;

    Timing::Time lastTime;
    Timing::Time lastDeltaTime; 
    
    Timing::Time maxAllowableDeltaTime; // if more time (in seconds) than this has passed, no PID adjustments will be made
};

//------------------------------------------------------------------------------
/**
*/
inline
PIDFeedbackLoop::PIDFeedbackLoop() : 
    value(0.0),
    goal(0.0),
    pConst(1.0),
    iConst(0.0),
    dConst(0.0),
    maxAcceleration(0.0),
    lastError(0.0),
    lastDelta(0.0),
    runningError(0.0),
    validError(false),
    isAngularValue(false),
    lastTime(0.0),
    lastDeltaTime(0.0),
    maxAllowableDeltaTime(0.03)
{
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
PIDFeedbackLoop::SetState(double value)
{ 
    this->value = value;
    lastError = 0.0;
    lastDelta = 0.0;
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
PIDFeedbackLoop::SetGoal(double wantedValue)
{ 
    this->goal = wantedValue; 
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
PIDFeedbackLoop::SetConstants(double pConst, double iConst, double dConst, double acceleration)
{ 
    this->pConst = pConst; 
    this->iConst = iConst; 
    this->dConst = dConst;
    maxAcceleration = acceleration;
}

//------------------------------------------------------------------------------
/**
*/
inline
const double& 
PIDFeedbackLoop::GetState() const
{
    return this->value;
}


//------------------------------------------------------------------------------
/**
*/
inline
double 
PIDFeedbackLoop::GetLastError() const
{
    return this->lastError;
}

//------------------------------------------------------------------------------
/**
*/
inline
const double& 
PIDFeedbackLoop::GetGoal() const
{
    return this->goal;
}

//------------------------------------------------------------------------------
/**
*/
inline
double 
PIDFeedbackLoop::GetLastDelta() const
{
    return this->lastDelta;
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
PIDFeedbackLoop::SetIsAngularValue(bool val)
{
    this->isAngularValue = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
PIDFeedbackLoop::Update(Timing::Time time) 
{ 
    Timing::Time frameTime = time - this->lastTime;
    // if too much time has passed, do nothing
    if (frameTime != 0.0f)
    {
        if (frameTime > maxAllowableDeltaTime)
            frameTime = maxAllowableDeltaTime;

        // compute the error and sum of the errors for the integral
        double difference;
        if (this->isAngularValue)
        {
            difference = (double)n_angulardistance((float)value, (float)goal);
        }
        else
        {
            difference = goal - value;
        }
        double error = difference * frameTime;   
        runningError += error;

        // proportional
        double dP = pConst * error;

        // integral
        double dI = iConst * runningError * frameTime;

        // derivative
        double dD(0.0f);
        if (validError)
            dD = dConst * (lastError - error) * frameTime;
        else
            validError = true;

        // remember the error for derivative
        lastError = error;

        // compute the adjustment
        double thisDelta = dP + dI + dD;

        // clamp the acceleration
        if (maxAcceleration != 0.0f)
        {
            double timeRatio(1.0);
            if (lastDeltaTime != 0.0)
                timeRatio = frameTime / lastDeltaTime;
            lastDeltaTime = frameTime;

            lastDelta *= timeRatio;
            double difference = (thisDelta - lastDelta);
            double accl = maxAcceleration * frameTime * frameTime;

            if (difference < -accl)
                thisDelta = lastDelta - accl;
            else if (difference > accl)
                thisDelta = lastDelta + accl;
        }

        // modify the value
        value += thisDelta;
        if (this->isAngularValue)
        {
            value = (double)n_modangle((float)value);
        }
        lastDelta = thisDelta;
    }
    this->lastTime = time;
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
PIDFeedbackLoop::ResetError()
{ 
    runningError = 0.0f;
    validError = false;
}
} // namespace Math
//------------------------------------------------------------------------------
#endif