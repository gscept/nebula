#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::Extrapolator

    Extrapolator maintains state about updates for remote entities, and 
    will generate smooth guesses about where those entities will be 
    based on previously received data.

    This implementation is based on http://www.mindcontrol.org/~hplus/epic/
    Its adapted for use with nebula classes.

    You create one Extrapolator per quantity you want to interpolate. 
    
    You then feed it updates about where your entity was ("positions") 
    at some point in time. Optionally, you can also give the Extrapolator
    information about the velocity at that point in time.
    After that, you can ask the Extrapolator for position values for 
    some given real time. Optionally, you can also ask for the velocity 
    at the same time.
    
    The Extrapolator will assume that an entity stops in place if it 
    hasn't received updates for some time about the entity. It will also 
    keep a running estimate of the latency and frequency of updates.
    
    Extrapolator requires a globally synchronized clock. It does not do 
    any management to deal with clock skew.

    @copyright
    (C) 2006 Jon Watte
    (C) 2009 Radon Labs GmbH
    (C) 2013-2025 Individual contributors, see AUTHORS file 
*/
#include "timing/time.h"

namespace Math
{
template<class TYPE> class Extrapolator
{
public:
    /// constructor
    Extrapolator();
    /// destructor
    ~Extrapolator() = default;
    /// add sample without velocity, velocity is compute from positions 
    bool AddSample(Timing::Time packetTime, Timing::Time curTime, const TYPE& pos);
    /// add sample with given velocity
    bool AddSample(Timing::Time packetTime, Timing::Time curTime, const TYPE& pos, const TYPE& vel);
    /// Re-set the Extrapolator's idea of time, velocity and position.
    void Reset(Timing::Time packetTime, Timing::Time curTime, const TYPE& pos);
    /// Re-set the Extrapolator's idea of time, velocity and position.
    void Reset(Timing::Time packetTime, Timing::Time curTime, const TYPE& pos, const TYPE& vel);
    /// Return an estimate of the interpolated position at a given global 
    /// time (which typically will be greater than the curTime passed into 
    /// AddSample()).
    bool ReadValue(Timing::Time forTime, TYPE& oPos) const;
    /// Return an estimate of the interpolated position at a given global 
    /// time (which typically will be greater than the curTime passed into 
    /// AddSample()).
    bool ReadValue(Timing::Time forTime, TYPE& oPos, TYPE& oVel) const;
    /// \return the current estimation of latency between the sender and this interpolator.
    Timing::Time EstimateLatency() const;
    /// \return the current estimation of frequency of updates that the sender will send.
    Timing::Time EstimateUpdateTime() const;

    /// is this packet newer than already received
    bool Estimates(Timing::Time packetTime, Timing::Time curTime);

    TYPE snapPos;
    TYPE snapVel;
    TYPE aimPos;
    TYPE lastPacketPos;     //  only used when re-constituting velocity
    Timing::Time snapTime;               //  related to this->snapPos
    Timing::Time aimTime;                //  related to this->aimPos
    Timing::Time lastPacketTime;         //  related to this->lastPacketPos
    Timing::Time latency;
    Timing::Time updateTime;
}; 

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Extrapolator<TYPE>::Extrapolator()
{       
    this->Reset(0, 0, TYPE());
}

//------------------------------------------------------------------------------
/**
    When you receive data about a remote entity, call AddSample() to 
    tell the Extrapolator about it. The Extrapolator will throw away a 
    sample with a time that's before any other time it's already gotten, 
    and return false; else it will use the sample to improve its 
    interpolation guess, and return true.
    \param packetTime The globally synchronized time at which the 
    packet was sent (and thus the data valid).
    \param curTime The globally synchronized time at which you put 
    the data into the Extrapolator (i e, "now").
    \param pos The position sample valid for packetTime.
    \return true if packetTime is strictly greater than the previous 
    packetTime, else false.
*/
template<class TYPE> bool 
Extrapolator<TYPE>::AddSample(Timing::Time packetTime, Timing::Time curTime, const TYPE& pos)
{
    //  The best guess I can make for velocity is the difference between 
    //  this sample and the last registered sample.
    // if you use this function, TYPE must implement zero assignment 
    TYPE vel(0);
    if (Math::abs(packetTime - this->lastPacketTime) > N_TINY) // 1e-4
    {
        float dt = (float)(1.0 / (packetTime - this->lastPacketTime));
        vel = (pos - this->lastPacketPos) * dt;
    }
    return this->AddSample(packetTime, curTime, pos, vel);
}
     
//------------------------------------------------------------------------------
/**
    When you receive data about a remote entity, call AddSample() to 
    tell the Extrapolator about it. The Extrapolator will throw away a 
    sample with a time that's before any other time it's already gotten, 
    and return false; else it will use the sample to improve its 
    interpolation guess, and return true.
    
    If you get velocity information with your position updates, you can 
    make the guess that Extrapolator makes better, by passing that 
    information along with your position sample.
    \param packetTime The globally synchronized time at which the 
    packet was sent (and thus the data valid).
    \param curTime The globally synchronized time at which you put 
    the data into the Extrapolator (i e, "now").
    \param pos The position sample valid for packetTime.
    \return true if packetTime is strictly greater than the previous 
    packetTime, else false.
    \param vel The velocity of the entity at the time of packetTime.
    Used to improve the guess about entity position.
*/
template<class TYPE> bool 
Extrapolator<TYPE>::AddSample(Timing::Time packetTime, Timing::Time curTime, const TYPE& pos, const TYPE& vel)
{
    if (!Estimates(packetTime, curTime)) {
        return false;
    }
    this->lastPacketPos = pos;
    this->lastPacketTime = packetTime;
    this->ReadValue(curTime, this->snapPos);
    this->aimTime = curTime + this->updateTime;
    float dt = (float)(this->aimTime - packetTime);
    this->snapTime = curTime;
    this->aimPos = pos + vel * dt;
    
    //  I now have two positions and two times:
    //  this->aimPos / this->aimTime
    //  this->snapPos / this->snapTime
    //  I must generate the interpolation velocity based on these two samples.
    //  However, if this->aimTime is the same as this->snapTime, I'm in trouble. In that 
    //  case, use the supplied velocity.
    if (Math::abs(this->aimTime - this->snapTime) < N_TINY) 
    {       
        this->snapVel = vel;                
    }
    else 
    {
        float dt = (float)(1.0 / (this->aimTime - this->snapTime));
        this->snapVel = (this->aimPos - this->snapPos) * dt;        
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Re-set the Extrapolator's idea of time, velocity and position.
    \param packetTime The packet time to reset to.
    \param curTime The current time to reset to.
    \param pos The position to reset to.
    \note The velocity will be re-set to 0.
*/
template<class TYPE> void 
Extrapolator<TYPE>::Reset(Timing::Time packetTime, Timing::Time curTime, const TYPE& pos)
{
    TYPE vel(0);
    this->Reset(packetTime, curTime, pos, vel);
}

//------------------------------------------------------------------------------
/**
    Re-set the Extrapolator's idea of time, velocity and position.
    \param packetTime The packet time to reset to.
    \param curTime The current time to reset to.
    \param pos The position to reset to.
    \param vel The velocity to reset to.
*/
template<class TYPE> void 
Extrapolator<TYPE>::Reset(Timing::Time packetTime, Timing::Time curTime, const TYPE& pos, const TYPE& vel)
{
    n_assert(packetTime <= curTime);
    this->lastPacketTime = packetTime;
    this->lastPacketPos = pos;
    this->snapTime = curTime;
    this->snapPos = pos;
    this->updateTime = curTime - packetTime;
    this->latency = this->updateTime;
    this->aimTime = curTime + this->updateTime;
    this->snapVel = vel;    
    this->aimPos = this->snapPos + this->snapVel * (float)this->updateTime;    
}

//------------------------------------------------------------------------------
/**
    Return an estimate of the interpolated value at a given global 
    time (which typically will be greater than the curTime passed into 
    AddSample()).
    \param forTime The time at which to interpolate the entity. It should 
    be greater than the last packetTime, and less than the last curTime 
    plus some allowable slop (determined by EstimateFreqency()).
    \param oPos The interpolated value for the given time.
    \return false if forTime is out of range (at which point the oPos 
    will still make sense, but movement will be clamped); true when forTime
    is within range.
*/
template<class TYPE> bool 
Extrapolator<TYPE>::ReadValue(Timing::Time forTime, TYPE& oPos) const
{
    TYPE vel;
    return this->ReadValue(forTime, oPos, vel);
}

//------------------------------------------------------------------------------
/**
    Return an estimate of the interpolated value at a given global 
    time (which typically will be greater than the curTime passed into 
    AddSample()).
    \param forTime The time at which to interpolate the entity. It should 
    be greater than the last packetTime, and less than the last curTime 
    plus some allowable slop (determined by EstimateFreqency()).
    \param oPos The interpolated position for the given time.
    \param oVel The interpolated velocity for the given time.
    \return false if forTime is out of range (at which point the oPos 
    will still make sense, but velocity will be zero); true when forTime
    is within range.
*/
template<class TYPE> bool 
Extrapolator<TYPE>::ReadValue(Timing::Time forTime, TYPE& oPos, TYPE& oVel) const
{
    bool ok = true;

    //  asking for something before the allowable time?
    if (forTime < this->snapTime) 
    {
        forTime = this->snapTime;
        ok = false;
    }

    //  asking for something very far in the future?
    Timing::Time maxRange = this->aimTime + this->updateTime;
    if (forTime > maxRange) 
    {
        forTime = maxRange;
        ok = false;
    }

    //  calculate the interpolated position
    oVel = this->snapVel;
    oPos = this->snapPos + oVel * (float)(forTime - this->snapTime);

    return ok;
}

//------------------------------------------------------------------------------
/**
    \return the current estimation of latency between the sender and this
    interpolator. This is updated after each AddSample(), and re-set 
    on Reset().
*/
template<class TYPE> Timing::Time 
Extrapolator<TYPE>::EstimateLatency() const
{
    return this->latency;
}

//------------------------------------------------------------------------------
/**
    \return the current estimation of frequency of updates that the sender 
    will send. This is updated after each AddSample(), and re-set on Reset().
*/
template<class TYPE> Timing::Time 
Extrapolator<TYPE>::EstimateUpdateTime() const
{
    return this->updateTime;
}
    
//------------------------------------------------------------------------------
/**
*/
template<class TYPE> bool 
Extrapolator<TYPE>::Estimates(Timing::Time packet, Timing::Time cur)
{
    if (packet <= this->lastPacketTime) 
    {
        return false;
    }

    //  The theory is that, if latency increases, quickly 
    //  compensate for it, but if latency decreases, be a 
    //  little more resilient; this is intended to compensate 
    //  for jittery delivery.
    Timing::Time lat = cur - packet;
    if (lat < 0) lat = 0;
    if (lat > this->latency) 
    {
        this->latency = (this->latency + lat) * 0.5;
    }
    else 
    {
        this->latency = (this->latency * 7 + lat) * 0.125;
    }

    //  Do the same running average for update time.
    //  Again, the theory is that a lossy connection wants 
    //  an average of a higher update time.
    Timing::Time tick = packet - this->lastPacketTime;
    if (tick > this->updateTime) 
    {
        this->updateTime = (this->updateTime + tick) * 0.5;
    }
    else 
    {
        this->updateTime = (this->updateTime * 7.0 + tick) * 0.125;
    }

    return true;
}

}