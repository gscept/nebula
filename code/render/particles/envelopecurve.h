#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::EnvelopeCurve
    
    An Attack/Sustain/Release envelope curve class with optional
    sine/cosine modulation. Used for animated particle emitter attributes.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "math/scalar.h"
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Particles
{
class EnvelopeCurve
{
public:
    /// modulation enumerator
    enum ModFunc
    {
        Sine = 0,
        Cosine,
    };

    /// default constructor
    EnvelopeCurve();
    /// set parameters
    void Setup(float val0, float val1, float val2, float val3, float keyPos0, float keyPos1, float freq, float amp, ModFunc mod);
    /// sample at specific time (0..1)
    float Sample(float t) const;
    /// sample from t=0 to t=1 into array of values
    void PreSample(float* sampleBuffer, SizeT numSamples, SizeT sampleStride) const;
    /// get the max of val0, val1, val2, val3
    float GetMaxValue() const;
    /// get the min of val0, v1l, val2, val3
    float GetMinValue() const;
    /// get values
    const float* GetValues() const;
    /// set values
    void SetValues(float v0, float v1, float v2, float v3);
    /// get limits in y
    const float* GetLimits() const;
    /// set limits in y
    void SetLimits(float min, float max);
    /// get keypos0
    const float GetKeyPos0() const;
    /// set keypos0
    void SetKeyPos0(float f);
    /// get keypos1
    const float GetKeyPos1() const;
    /// set keypos1
    void SetKeyPos1(float f);
    /// get frequency
    const float GetFrequency() const;
    /// set frequency
    void SetFrequency(float f);
    /// get amplitude
    const float GetAmplitude() const;
    /// set amplitude
    void SetAmplitude(float f);
    /// get modFunction
    const int GetModFunc() const;
    /// set mod function
    void SetModFunc(int func);

private:
    float values[4];
    float limits[2];
    float keyPos0;
    float keyPos1;
    float frequency;
    float amplitude;
    ModFunc modFunction : 1;
};

//------------------------------------------------------------------------------
/**
*/
inline const float*
EnvelopeCurve::GetValues() const
{
    return this->values;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
EnvelopeCurve::SetValues( float v0, float v1, float v2, float v3 )
{
    this->values[0] = v0;
    this->values[1] = v1;
    this->values[2] = v2;
    this->values[3] = v3;
}

//------------------------------------------------------------------------------
/**
*/
inline const float
EnvelopeCurve::GetKeyPos0() const
{
    return this->keyPos0;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
EnvelopeCurve::SetKeyPos0( float f )
{
    this->keyPos0 = f;
}

//------------------------------------------------------------------------------
/**
*/
inline const float
EnvelopeCurve::GetKeyPos1() const
{
    return this->keyPos1;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
EnvelopeCurve::SetKeyPos1( float f )
{
    this->keyPos1 = f;
}

//------------------------------------------------------------------------------
/**
*/
inline const float
EnvelopeCurve::GetFrequency() const
{
    return this->frequency;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
EnvelopeCurve::SetFrequency( float f )
{
    this->frequency = f;
}

//------------------------------------------------------------------------------
/**
*/
inline const float
EnvelopeCurve::GetAmplitude() const
{
    return this->amplitude;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
EnvelopeCurve::SetAmplitude( float f )
{
    this->amplitude = f;
}

//------------------------------------------------------------------------------
/**
*/
inline const int
EnvelopeCurve::GetModFunc() const
{
    return this->modFunction;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
EnvelopeCurve::SetModFunc( int i )
{
    this->modFunction = (ModFunc)i;
}


//------------------------------------------------------------------------------
/**
*/
inline const float* 
EnvelopeCurve::GetLimits() const
{
    return this->limits;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
EnvelopeCurve::SetLimits( float min, float max )
{
    this->limits[0] = min;
    this->limits[1] = max;
}

} // namespace Particles
//------------------------------------------------------------------------------
