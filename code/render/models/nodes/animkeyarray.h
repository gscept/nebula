#pragma once
//------------------------------------------------------------------------------
/**
    @class AnimKeyArray

    Legacy N2 crap!
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "util/array.h"
#include "models/nodes/animkey.h"
#include "models/nodes/animlooptype.h"

//------------------------------------------------------------------------------
namespace Models
{
template<class TYPE> class AnimKeyArray : public Util::Array<TYPE>
{
public:
    /// constructor with default parameters
    AnimKeyArray();
    /// constuctor with initial size and grow size
    AnimKeyArray(int initialSize, int initialGrow);
    /// get sampled key
    bool Sample(float sampleTime, AnimLoopType::Type loopType, TYPE& result);
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
AnimKeyArray<TYPE>::AnimKeyArray() :
Util::Array<TYPE>()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
AnimKeyArray<TYPE>::AnimKeyArray(int initialSize, int initialGrow) :
Util::Array<TYPE>(initialSize, initialGrow)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
AnimKeyArray<TYPE>::Sample(float sampleTime, AnimLoopType::Type loopType, TYPE& result)
{
    if (this->Size() > 1)
    {
        float minTime = this->Front().GetTime();
        float maxTime = this->Back().GetTime();
        if (maxTime > 0.0f)
        {
            if (AnimLoopType::Loop == loopType)
            {
                // in loop mode, wrap time into loop time
                sampleTime = sampleTime - (float(floor(sampleTime / maxTime)) * maxTime);
            }

            // clamp time to range
            if (sampleTime < minTime)       sampleTime = minTime;
            else if (sampleTime >= maxTime) sampleTime = maxTime - 0.001f;

            // find the surrounding keys
            n_assert(this->Front().GetTime() == 0.0f);
            int i = 0;
            while ((*this)[i].GetTime() <= sampleTime)
            {
                i++;
            }
            n_assert((i > 0) && (i < this->Size()));

            const TYPE& key0 = (*this)[i - 1];
            const TYPE& key1 = (*this)[i];
            float time0 = key0.GetTime();
            float time1 = key1.GetTime();

            // compute the actual interpolated values
            float lerpTime;
            if (time1 > time0) lerpTime = (float) ((sampleTime - time0) / (time1 - time0));
            else               lerpTime = 1.0f;

            result.Lerp(key0.GetValue(), key1.GetValue(), lerpTime);
            result.SetTime(sampleTime);
            return true; 
        }
    }
    return false;
}

};
