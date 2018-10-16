//------------------------------------------------------------------------------
//  blob.cc
//  (C) 2006 RadonLabs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/blob.h"
#include <string.h>
#include "base64/base64.h"

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

//------------------------------------------------------------------------------
/**
    decodes a base64 buffer and stores it inside
*/
void
Blob::SetFromBase64(const void* ptr, SizeT size)
{
    SizeT allocsize = BASE64_DECODE_OUT_SIZE(size);
    this->Reserve(allocsize);    
    int ret = base64_decode((char*)ptr, size, (unsigned char*)this->ptr);    
    n_assert(ret >= 0);
    this->size = ret;
}

//------------------------------------------------------------------------------
/**
    creates a base64 copy blob
*/
Util::Blob 
Blob::GetBase64() const
{    
    SizeT allocsize = BASE64_ENCODE_OUT_SIZE(this->size);    
    Util::Blob ret(allocsize);
    int enc = base64_encode((unsigned char*)this->ptr, this->size, (char*)ret.ptr);
    n_assert(enc >= 0);
    ret.size = enc;
    return ret;
}

}