#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::StreamCache
    
    Explicit cache for reusing read-only streams that are already opened.
    Main use-case are expensive to open streams like http objects/zips
    
    (C) 2020 Individual contributors, see AUTHORS file
*/

#include "core/refcounted.h"
#include "core/singleton.h"
#include "io/uri.h"
#include "io/stream.h"
#include "util/dictionary.h"

//------------------------------------------------------------------------------
namespace IO
{
class StreamCache : public Core::RefCounted
{
    __DeclareClass(StreamCache);
    __DeclareSingleton(StreamCache);
public:
    /// constructor
    StreamCache();
    /// destructor
    virtual ~StreamCache();

    ///
    bool IsCached(IO::URI const& uri) const;
    /// 
    Util::KeyValuePair<void*,Ptr<IO::Stream>> GetCachedStream(IO::URI const& uri);
    ///
    bool OpenStream(IO::URI const& uri, Core::Rtti const& rtti);
    ///
    void RemoveStream(IO::URI const& uri);

private:
    /// discard 
    void Discard();
    struct CacheEntry
    {
        Ptr<IO::Stream> stream;
        void * buffer;
        SizeT useCount;
    };

    Util::Dictionary<Util::String, CacheEntry> streams;
};

} // namespace IO
//------------------------------------------------------------------------------
