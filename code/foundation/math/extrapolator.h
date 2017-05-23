#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::Extrapolator

    Extrapolator maintains state about updates for remote entities, and 
    will generate smooth guesses about where those entities will be 
    based on previously received data.

    This implementation is based on http://www.mindcontrol.org/~hplus/epic/
    Its adapted for use with point and vector class.

	(C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file	
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
	virtual ~Extrapolator();
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

private:                                                      
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
*/
template<class TYPE>
Extrapolator<TYPE>::~Extrapolator()
{
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> bool 
Extrapolator<TYPE>::AddSample(Timing::Time packetTime, Timing::Time curTime, const TYPE& pos)
{
    //  The best guess I can make for velocity is the difference between 
    //  this sample and the last registered sample.
    // if you use this function, TYPE must implement zero assignment 
    TYPE vel(0);
    if (n_abs(packetTime - this->lastPacketTime) > N_TINY) //1e-4
    {
        float dt = (float)(1.0 / (packetTime - this->lastPacketTime));
        vel = (pos - this->lastPacketPos) * dt;             
    }    
    return this->AddSample(packetTime, curTime, pos, vel);
}
     
//------------------------------------------------------------------------------
/**
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
    if (n_abs(this->aimTime - this->snapTime) < N_TINY) 
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
*/
template<class TYPE> void 
Extrapolator<TYPE>::Reset(Timing::Time packetTime, Timing::Time curTime, const TYPE& pos)
{
    TYPE vel;    
    this->Reset(packetTime, curTime, pos, vel);
}

//------------------------------------------------------------------------------
/**
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
*/
template<class TYPE> bool 
Extrapolator<TYPE>::ReadValue(Timing::Time forTime, TYPE& oPos) const
{
    TYPE vel;
    return this->ReadValue(forTime, oPos, vel);
}

//------------------------------------------------------------------------------
/**
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
*/
template<class TYPE> Timing::Time 
Extrapolator<TYPE>::EstimateLatency() const
{
    return this->latency;
}

//------------------------------------------------------------------------------
/**
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
        this->updateTime = (this->updateTime * 7 + tick) * 0.125;
    }

    return true;
}

}