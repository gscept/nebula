//------------------------------------------------------------------------------
//  envelopecurve.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
#include "render/stdneb.h"
#include "particles/envelopecurve.h"

//------------------------------------------------------------------------------
namespace Particles
{

//------------------------------------------------------------------------------
/**
*/
EnvelopeCurve::EnvelopeCurve() :
    keyPos0(0.33f),
    keyPos1(0.66f),
    frequency(0.0f),
    amplitude(0.0f),
    modFunction(Sine)
{
    this->values[0] = 0.0f;
    this->values[1] = 0.0f;
    this->values[2] = 0.0f;
    this->values[3] = 0.0f;

	this->limits[0] = 0.0f;
	this->limits[1] = 1.0f;
} 

//------------------------------------------------------------------------------
/**
*/
void
EnvelopeCurve::Setup(float val0, float val1, float val2, float val3, float keyp0, float keyp1, float freq, float amp, ModFunc mod)
{
    this->values[0] = val0;
    this->values[1] = val1;
    this->values[2] = val2;
    this->values[3] = val3;
    this->keyPos0 = keyp0;
    this->keyPos1 = keyp1;
    this->frequency = freq;
    this->amplitude = amp;
    this->modFunction = mod;
}

//------------------------------------------------------------------------------
/**
    NOTE: Sampling a single value is relatively expensive. Consider
    pre-sampling into a lookup table!
*/
float
EnvelopeCurve::Sample(float t) const
{
    t = Math::n_saturate(t);

    float value;
    if (t < this->keyPos0)
    {
        value = Math::n_lerp(this->values[0], this->values[1], t / this->keyPos0);
    }
    else if (t < this->keyPos1)
    {
        value = Math::n_lerp(this->values[1], this->values[2], (t - this->keyPos0) / (this->keyPos1 - this->keyPos0));
    }
    else
    {
        value = Math::n_lerp(this->values[2], this->values[3], (t - this->keyPos1) / (1.0f - this->keyPos1));
    }
    if (this->amplitude > 0.0f)
    {
        if (Sine == this->modFunction)
        {
            value += Math::n_sin(t * 2.0f * N_PI * this->frequency) * this->amplitude;
        }
        else
        {
            value += Math::n_cos(t * 2.0f * N_PI * this->frequency) * this->amplitude;
        }
    }
    return value;
}

//------------------------------------------------------------------------------
/**
    This samples N values from t=0 to t=1 into an array. The array can then
    be used as a lookup table.
*/
void
EnvelopeCurve::PreSample(float* sampleBuffer, SizeT numSamples, SizeT sampleStride) const
{
    float t = 0.0f;
    float d = 1.0f / numSamples;
    IndexT sampleIndex = 0;
    IndexT i;
    for (i = 0; i < numSamples; i++)
    {
        sampleBuffer[sampleIndex] = this->Sample(t);
        sampleIndex += sampleStride;
        t += d;
    }
}

//------------------------------------------------------------------------------
/**
*/
float
EnvelopeCurve::GetMaxValue() const
{
    return Math::n_max(this->values[0], Math::n_max(this->values[1], Math::n_max(this->values[2], this->values[3])));
}

//------------------------------------------------------------------------------
/**
*/
float 
EnvelopeCurve::GetMinValue() const
{
	return Math::n_min(this->values[0], Math::n_min(this->values[1], Math::n_min(this->values[2], this->values[3])));
}


} // namespace Particles
//------------------------------------------------------------------------------
