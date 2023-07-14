//------------------------------------------------------------------------------
//  streamcache.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/cache/streamcache.h"

namespace IO
{
__ImplementClass(IO::StreamCache, 'STCA', Core::RefCounted);
__ImplementSingleton(IO::StreamCache);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
StreamCache::StreamCache()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
StreamCache::~StreamCache()
{
    this->Discard();
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool 
StreamCache::IsCached(IO::URI const & uri) const
{
    return this->streams.Contains(uri.AsString());
}

//------------------------------------------------------------------------------
/**
*/
Util::KeyValuePair<void*, Ptr<IO::Stream>>
StreamCache::GetCachedStream(IO::URI const & uri)
{
    n_assert(this->IsCached(uri));
    CacheEntry& entry = this->streams[uri.AsString()];
    ++entry.useCount;
    return Util::KeyValuePair(entry.buffer, entry.stream);
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamCache::OpenStream(IO::URI const& uri, Core::Rtti const& rtti)
{
    String uriString = uri.AsString();
    n_assert(!this->streams.Contains(uriString));

    Ptr<IO::Stream> stream = (Stream*) rtti.Create();

    stream->SetURI(uri);
    stream->SetAccessMode(Stream::ReadAccess);
    if (stream->Open())
    {
        CacheEntry & entry = this->streams.Emplace(uriString);
        entry.stream = stream;
        entry.useCount = 0;
        entry.buffer = stream->Map();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void 
StreamCache::RemoveStream(IO::URI const & uri)
{
    Util::String uriString = uri.AsString();
    n_assert(this->streams.Contains(uriString));
    CacheEntry& entry = this->streams[uriString];
    --entry.useCount;
    if (entry.useCount == 0)
    {
        entry.stream->Unmap();
        entry.stream->Close();
        entry.stream = nullptr;
        this->streams.Erase(uriString);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
StreamCache::Discard()
{
    // release all streams
    for (CacheEntry& entry : this->streams.ValuesAsArray())
    {
        entry.stream->Unmap();
        entry.stream->Close();
        entry.stream = nullptr;
    }
    this->streams.Clear();
}

} // namespace IO
