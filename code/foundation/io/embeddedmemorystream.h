#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::EmbeddedMemoryStream
    
    Implements a stream class which reads from an embedded resource 
    
    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
#include "io/memorystream.h"

//------------------------------------------------------------------------------
namespace IO
{
class EmbeddedMemoryStream : public MemoryStream
{
    __DeclareClass(EmbeddedMemoryStream);
public:
    /// constructor
    EmbeddedMemoryStream();
    /// destructor
    virtual ~EmbeddedMemoryStream();
    /// memory streams support writing
    virtual bool CanWrite() const;
    /// open the stream
    virtual bool Open();
};

} // namespace IO
//------------------------------------------------------------------------------
