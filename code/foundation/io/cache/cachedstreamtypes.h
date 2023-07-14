#pragma once
//------------------------------------------------------------------------------
/**
    Instances of wrapped stream classes
    
    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/    
#include "core/config.h"
#include "io/cache/cachedstream.h"

namespace IO
{

class CachedHttpStream : public CachedStream
{
    __DeclareClass(CachedHttpStream);

protected:
    virtual Core::Rtti const& GetParentRtti();
};

}
