#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::StringBuffer
  
    Global string buffer for the StringAtom system. This is where all
    raw strings for the StringAtom system are stored. If enabled,
    the StringBuffer can grow, but it may never shrink.
    Once a string is in the string buffer,
    it cannot be removed. String data is simply appended to the last
    position, strings are separated by a 0-terminator-byte. A string
    is guaranteed never to move in memory. Several threads can 
    have simultaneous read-access to the string buffer, even while
    an AddString() is in progress by another thread. Only if several
    threads attempt to call AddString() a lock must be taken.

    NOTE: NOT thread-safe! Usually, GlobalStringAtomTable cares
    about thread-safety for the global string buffer.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/array.h"

//------------------------------------------------------------------------------
namespace Util
{
class StringBuffer
{
public:
    /// constructor
    StringBuffer();
    /// destructor
    ~StringBuffer();
    
    /// setup the string buffer with size in bytes
    void Setup(SizeT size);
    /// discard the string buffer
    void Discard();
    /// return true if string buffer has been setup
    bool IsValid() const;

    /// add a string to the end of the string buffer, return pointer to string
    const char* AddString(const char* str);
    /// DEBUG: return next string in string buffer
    const char* NextString(const char* prev);
    /// DEBUG: get number of allocated chunks
    SizeT GetNumChunks() const;

private:
    /// allocate a new chunk
    void AllocNewChunk();

    Util::Array<char*> chunks;
    SizeT chunkSize;    
    char* curPointer;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
StringBuffer::IsValid() const
{
    return (0 != this->curPointer);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
StringBuffer::GetNumChunks() const
{
    return this->chunks.Size();
}

} // namespace Util;
//------------------------------------------------------------------------------
