#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::IndexBufferBase
  
    A resource which holds an array of indices into an array of vertices.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "coregraphics/indextype.h"

//------------------------------------------------------------------------------
namespace Base
{
class IndexBufferBase
{
public:

	struct IndexBufferBaseInfo
	{
		int indexCount;
	};
};

} // namespace Base
//------------------------------------------------------------------------------

