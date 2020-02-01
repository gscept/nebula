//------------------------------------------------------------------------------
//  primitivetopology.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/primitivetopology.h"

namespace CoreGraphics
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
PrimitiveTopology::Code
PrimitiveTopology::FromString(const String& str)
{
    if (str == "PointList")             return PointList;
    else if (str == "LineList")         return LineList;
    else if (str == "LineStrip")        return LineStrip;
    else if (str == "TriangleList")     return TriangleList;
    else if (str == "TriangleStrip")    return TriangleStrip;
	else if (str == "PatchList")		return PatchList;

    else
    {
        n_error("PrimitiveTopology::FromString(): invalid string '%s'!", str.AsCharPtr());
        return InvalidPrimitiveTopology;
    }
}

//------------------------------------------------------------------------------
/**
*/
String
PrimitiveTopology::ToString(Code code)
{
    switch (code)
    {
        case PointList:     return "PointList";
        case LineList:      return "LineList";
        case LineStrip:     return "LineStrip";
        case TriangleList:  return "TriangleList";
        case TriangleStrip: return "TriangleStrip";
		case PatchList:		return "PatchList";

        default:
            n_error("PrimitiveTopology::ToString(): invalid topology code!");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
    Computes the number of required vertices for a given primitive
    topology and number of primitives.
*/
SizeT
PrimitiveTopology::NumberOfVertices(Code topology, SizeT numPrimitives)
{
    switch (topology)
    {
        case PointList:     return numPrimitives;
        case LineList:      return numPrimitives * 2;
        case LineStrip:     return numPrimitives + 1;
        case TriangleList:  return numPrimitives * 3;
        case TriangleStrip: return numPrimitives + 2;
		case PatchList:		return numPrimitives;
        
        default:
            n_error("PrimitiveTopology::NumberOfVertices(): invalid topology!");
            return InvalidIndex;
    }
}

//------------------------------------------------------------------------------
/**
    Computes the number of primitives from a given primitive topology
    and number of vertices (the opposite of ComputeNumberOfVertices()).
*/
SizeT
PrimitiveTopology::NumberOfPrimitives(Code topology, SizeT numVertices)
{
    switch (topology)
    {
        case PointList:     return numVertices;
        case LineList:      return numVertices / 2;
        case LineStrip:     return numVertices - 1;
        case TriangleList:  return numVertices / 3;
        case TriangleStrip: return numVertices - 2;
		case PatchList:		return numVertices;

        default:
            n_error("PrimitiveTopology::NumberOfPrimitives(): invalid topology!");
            return InvalidIndex;
    }
}

} // namespace CoreGraphics
