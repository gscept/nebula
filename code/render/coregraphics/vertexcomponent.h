#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::VertexComponent
  
    Describes a single vertex component in a vertex layout description.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#if __VULKAN__
#include "coregraphics/base/vertexcomponentbase.h"
namespace CoreGraphics
{
class VertexComponent : public Base::VertexComponentBase
{
public:
    /// default constructor
    VertexComponent() { };
    /// constructor
    VertexComponent(SemanticName semName, IndexT semIndex, Format format, IndexT streamIndex=0, StrideType strideType=PerVertex, SizeT stride=0) : VertexComponentBase(semName, semIndex, format, streamIndex, strideType, stride) { };
    /// constructor with slot instead of semantic
    VertexComponent(IndexT slot, IndexT semIndex, Format format, IndexT streamIndex = 0, StrideType strideType = PerVertex, SizeT stride = 0) : VertexComponentBase((SemanticName)slot, semIndex, format, streamIndex, strideType, stride) { };
};
}
#else
#error "CoreGraphics::VertexComponent not implemented on this platform"
#endif
