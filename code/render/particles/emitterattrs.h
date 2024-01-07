#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::EmitterAttrs
    
    A container for particle emitter attributes.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "particles/envelopecurve.h"
#include "math/vec4.h"

//------------------------------------------------------------------------------
namespace Particles
{
class EmitterAttrs
{
public:
    /// scalar attributes
    enum FloatAttr
    {
        EmissionDuration = 0,
        ActivityDistance,
        StartRotationMin,
        StartRotationMax,
        Gravity,
        ParticleStretch,
        VelocityRandomize,
        RotationRandomize,
        SizeRandomize,
        PrecalcTime,
        StartDelay,
        TextureTile,
        PhasesPerSecond,

        NumFloatAttrs,
    };
    
    /// boolean attributes
    enum BoolAttr
    {
        Looping = 0,
        RandomizeRotation,
        StretchToStart,
        RenderOldestFirst,
        ViewAngleFade,
        Billboard,

        NumBoolAttrs,
    };

    /// integer attributes
    enum IntAttr
    {
        StretchDetail = 0,
        AnimPhases,

        NumIntAttrs,
    };

    /// vec4 attributes
    enum Float4Attr
    {
        WindDirection = 0,

        NumFloat4Attrs
    };
    
    /// scalar envelope attributes
    enum EnvelopeAttr
    {
        Red = 0,
        Green,
        Blue,
        Alpha,
        EmissionFrequency,
        LifeTime,
        StartVelocity,
        RotationVelocity,
        Size,
        SpreadMin,
        SpreadMax,
        AirResistance,
        VelocityFactor,
        Mass,
        TimeManipulator,

        Alignment0,             // pad value to align num curves to 16
        
        NumEnvelopeAttrs,
    };

    /// constructor
    EmitterAttrs();
    /// set float attribute
    void SetFloat(FloatAttr key, float value);
    /// get float attribute
    float GetFloat(FloatAttr key) const;
    /// set bool attribute
    void SetBool(BoolAttr key, bool value);
    /// get bool attribute
    bool GetBool(BoolAttr key) const;
    /// set int attribute
    void SetInt(IntAttr key, int value);
    /// get int attribute
    int GetInt(IntAttr key) const;
    /// set vec4 attribute
    void SetVec4(Float4Attr key, const Math::vec4& value);
    /// get vec4 attribute
    const Math::vec4& GetVec4(Float4Attr key) const;
    /// set envelope attribute
    void SetEnvelope(EnvelopeAttr key, const EnvelopeCurve& value);
    /// get envelope attribute
    const EnvelopeCurve& GetEnvelope(EnvelopeAttr key) const;

private:
    Math::vec4 vec4Values[NumFloat4Attrs];
    float floatValues[NumFloatAttrs];
    EnvelopeCurve envelopeValues[NumEnvelopeAttrs];

    struct IntAttributes
    {
        uint stretchDetail : 8;
        uint animPhases : 8;
    } intAttributes;

    struct BoolAttributes
    {
        bool looping : 1;
        bool randomizeRotation : 1;
        bool stretchToStart : 1;
        bool renderOldestFirst : 1;
        bool viewAngleFade : 1;
        bool billboard : 1;
    } boolAttributes;
};

//------------------------------------------------------------------------------
/**
*/
inline void
EmitterAttrs::SetFloat(FloatAttr key, float value)
{
    this->floatValues[key] = value;
}

//------------------------------------------------------------------------------
/**
*/
inline float
EmitterAttrs::GetFloat(FloatAttr key) const
{
    return this->floatValues[key];
}

//------------------------------------------------------------------------------
/**
*/
inline void
EmitterAttrs::SetBool(BoolAttr key, bool value)
{
    switch (key)
    {
    case Looping:
        this->boolAttributes.looping = value;
        break;
    case RandomizeRotation:
        this->boolAttributes.randomizeRotation = value;
        break;
    case StretchToStart:
        this->boolAttributes.stretchToStart = value;
        break;
    case RenderOldestFirst:
        this->boolAttributes.renderOldestFirst = value;
        break;
    case ViewAngleFade:
        this->boolAttributes.viewAngleFade = value;
        break;
    case Billboard:
        this->boolAttributes.billboard = value;
        break;
    default: n_error("unhandled enum"); break;
    };
}

//------------------------------------------------------------------------------
/**
*/
inline bool
EmitterAttrs::GetBool(BoolAttr key) const
{
    switch (key)
    {
    case Looping:
        return this->boolAttributes.looping;
    case RandomizeRotation:
        return this->boolAttributes.randomizeRotation;
    case StretchToStart:
        return this->boolAttributes.stretchToStart;
    case RenderOldestFirst:
        return this->boolAttributes.renderOldestFirst;
    case ViewAngleFade:
        return this->boolAttributes.viewAngleFade;
    case Billboard:
        return this->boolAttributes.billboard;
    default: n_error("unhandled enum"); break;
    };
    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline void
EmitterAttrs::SetInt(IntAttr key, int value)
{
    switch (key)
    {
    case StretchDetail:
        this->intAttributes.stretchDetail = value;
        break;
    case AnimPhases:
        this->intAttributes.stretchDetail = value;
        break;
    default: n_error("unhandled enum"); break;
    };
}

//------------------------------------------------------------------------------
/**
*/
inline int
EmitterAttrs::GetInt(IntAttr key) const
{
    switch (key)
    {
    case StretchDetail:
        return this->intAttributes.stretchDetail;
    case AnimPhases:
        return this->intAttributes.stretchDetail;
    default: n_error("unhandled enum"); break;        
    };
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
EmitterAttrs::SetVec4( Float4Attr key, const Math::vec4& value )
{
    this->vec4Values[key] = value;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vec4& 
EmitterAttrs::GetVec4( Float4Attr key ) const
{
    return this->vec4Values[key];
}

//------------------------------------------------------------------------------
/**
*/
inline void
EmitterAttrs::SetEnvelope(EnvelopeAttr key, const EnvelopeCurve& value)
{
    this->envelopeValues[key] = value;
}

//------------------------------------------------------------------------------
/**
*/
inline const EnvelopeCurve&
EmitterAttrs::GetEnvelope(EnvelopeAttr key) const
{
    return this->envelopeValues[key];
}

} // namespace Particles
//------------------------------------------------------------------------------
    