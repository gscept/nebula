//------------------------------------------------------------------------------
//  cachedstreamtypes.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/cache/cachedstreamtypes.h"
#include "http/httpstream.h"

namespace IO
{
__ImplementClass(IO::CachedHttpStream, 'CHTP', IO::Stream);

//------------------------------------------------------------------------------
/**
*/
Core::Rtti const & 
CachedHttpStream::GetParentRtti()
{
    return Http::HttpStream::RTTI;
}

}
