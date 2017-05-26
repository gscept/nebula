#pragma once
//------------------------------------------------------------------------------
/**
    @class AnimKey
    @ingroup Util

    Associate a data value with a point in time.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Models
{
template<class TYPE> class AnimKey
{
public:
    /// default constructor
    AnimKey();
    /// construct with time and value
    AnimKey(float t, const TYPE& v);
    /// copy constructor
    AnimKey(const AnimKey& rhs);
    /// set time
    void SetTime(float t);
    /// get time
    float GetTime() const;
    /// set value
    void SetValue(const TYPE& v);
    /// get value
    const TYPE& GetValue() const;
    /// set to interpolated value
    void Lerp(const TYPE& key0, const TYPE& key1, float l);
    /// only compares time
    bool operator< (const AnimKey& right) const;
    /// only compares time
    bool operator> (const AnimKey& right) const;
    /// only compares time
    bool operator<=(const AnimKey& right) const;
    /// only compares time
    bool operator>=(const AnimKey& right) const;
    /// only compares time
    bool operator==(const AnimKey& right) const;
    /// only compares time
    bool operator!=(const AnimKey& right) const;

private:
    float time;
    TYPE value;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
AnimKey<TYPE>::AnimKey() :
    time(0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
AnimKey<TYPE>::AnimKey(float t, const TYPE& v) :
    time(t),
    value(v)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
AnimKey<TYPE>::AnimKey(const AnimKey& rhs) :
    time(rhs.time),
    value(rhs.value)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
AnimKey<TYPE>::SetTime(float t)
{
    this->time = t;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
float
AnimKey<TYPE>::GetTime() const
{
    return this->time;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
AnimKey<TYPE>::SetValue(const TYPE& v)
{
    this->value = v;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
const TYPE&
AnimKey<TYPE>::GetValue() const
{
    return this->value;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
AnimKey<TYPE>::Lerp(const TYPE& key0, const TYPE& key1, float l)
{
    this->value = (TYPE) (key0 + ((key1 - key0) * l));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline
bool AnimKey<TYPE>::operator<(const AnimKey<TYPE>& right) const
{
    return this->GetTime() < right.GetTime();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline
bool AnimKey<TYPE>::operator>(const AnimKey<TYPE>& right) const
{
    return this->GetTime() > right.GetTime();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline
bool AnimKey<TYPE>::operator<=(const AnimKey<TYPE>& right) const
{
    return this->GetTime() <= right.GetTime();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline
bool AnimKey<TYPE>::operator>=(const AnimKey<TYPE>& right) const
{
    return this->GetTime() >= right.GetTime();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline
bool AnimKey<TYPE>::operator==(const AnimKey<TYPE>& right) const
{
    return this->GetTime() == right.GetTime();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
inline
bool AnimKey<TYPE>::operator!=(const AnimKey<TYPE>& right) const
{
    return this->GetTime() != right.GetTime();
}

};
