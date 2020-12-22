#pragma once
//------------------------------------------------------------------------------
/**
    @class Frame::FrameBatchType
    
    Frame batch type hints for the render device and render targets. Indicates
    the type of objects that are rendered in a frame batch.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "util/string.h"

namespace Frame
{
class FrameBatchType
{
public:
    /// batch type enum
    enum Code
    {
        Depth,
        Solid,
        Alpha,
        Geometry,           // most models and stuff in here
        Shapes,             // ShapeRenderer shapes
        Text,               // TextRenderer elements
        UI,                 // user interface stuff
        ResolveDepthBuffer, // resolve depth buffer to texture
        MousePointers,      // MousePointers
        Lights,             // render lights
        LightProbes,        // render light probes
        System,             // called by the system
    
        InvalidBatchType
    };
    
    /// convert from string
    static FrameBatchType::Code FromString(const Util::String& str);
    /// convert to string
    static Util::String ToString(FrameBatchType::Code c);
};

//------------------------------------------------------------------------------
/**
*/
inline FrameBatchType::Code
FrameBatchType::FromString(const Util::String& str)
{
    if (str == "Depth")                    return Depth;
    else if (str == "Solid")               return Solid;
    else if (str == "Alpha")               return Alpha;
    else if (str == "Geometry")            return Geometry;
    else if (str == "Shapes")              return Shapes;
    else if (str == "Text")                return Text;
    else if (str == "UI")                  return UI;
    else if (str == "ResolveDepthBuffer")  return ResolveDepthBuffer;
    else if (str == "MousePointers")       return MousePointers;
    else if (str == "Lights")              return Lights;
    else if (str == "LightProbes")         return LightProbes;
    else
    {
        n_error("BatchType::FromString(): invalid string '%s'!", str.AsCharPtr());
        return InvalidBatchType;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
FrameBatchType::ToString(FrameBatchType::Code c)
{
    switch (c)
    {
        case Depth:                 return "Depth";
        case Solid:                 return "Solid";
        case Alpha:                 return "Alpha";
        case Geometry:              return "Geometry";
        case Shapes:                return "Shapes";
        case Text:                  return "Text";
        case UI:                    return "UI";
        case ResolveDepthBuffer:    return "ResolveDepthBuffer";
        case MousePointers:         return "MousePointers";
        case Lights:                return "Lights";
        case LightProbes:           return "LightProbes";
        default:
            n_error("BatchType::ToString(): invalid batch type!");
            return "";
    }
}

} // namespace Frame
//------------------------------------------------------------------------------

    