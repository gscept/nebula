//------------------------------------------------------------------------------
//  blob.cc
//  (C) 2006 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "util/blob.h"
#include <string.h>

namespace Util
{
Memory::Heap* Blob::DataHeap = 0;

//------------------------------------------------------------------------------
/**
    Like strcmp(), but checks the blob contents.
*/
int
Blob::BinaryCompare(const Blob& rhs) const
{
    n_assert(0 != this->ptr);
    n_assert(0 != rhs.ptr);
    if (this->size == rhs.size)
    {
        return memcmp(this->ptr, rhs.ptr, this->size);
    }
    else if (this->size > rhs.size)
    {
        return 1;
    }
    else
    {
        return -1;
    }
}

}