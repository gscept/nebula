#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::VertexBufferBase
  
    A resource which holds an array of vertices.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "coregraphics/vertexlayout.h"

//------------------------------------------------------------------------------
namespace Base
{
class VertexBufferBase
{
public:
    /// constructor
    VertexBufferBase();
    /// destructor
    virtual ~VertexBufferBase();

	struct VertexBufferBaseInfo
	{
		int vertexCount;
		int vertexByteSize;
	};
	typedef Resources::ResourceId VertexLayoutId;
};

} // namespace Base
//------------------------------------------------------------------------------

