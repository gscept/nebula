#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::SafeFileStream
  
    Wrapper around FileStream that will save to a temporary file and swap
    when closed
        
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
#include "io/filestream.h"
#include "util/string.h"
#include "io/filetime.h"
#include "io/fswrapper.h"

//------------------------------------------------------------------------------
namespace IO
{
class SafeFileStream : public FileStream
{
    __DeclareClass(SafeFileStream);
public:
    /// constructor
    SafeFileStream();
    /// destructor
    virtual ~SafeFileStream();    
    /// open the stream
    virtual bool Open();
    /// close the stream
    virtual void Close();
protected:
    IO::URI tmpUri;
};

} // namespace IO
//------------------------------------------------------------------------------
