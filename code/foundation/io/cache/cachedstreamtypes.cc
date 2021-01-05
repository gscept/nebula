//------------------------------------------------------------------------------
//  cachedstreamtypes.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/cache/cachedstreamtypes.h"
#include "io/cache/streamcache.h"
#include "io/cache/cachedstream.h"
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