#pragma once
//------------------------------------------------------------------------------
/** 
    @class Util::RunLengthCodec
    
    A simple byte-based runlength encoder/decoder. Note that the encoded
    size may actually be bigger then the original size for random data!
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{
class RunLengthCodec
{
public:
    /// get a safe size for the destination buffer
    static SizeT GetSafeRLEBufferSize(SizeT srcBufferSize);
    /// compute the decoded byte size from an RLE encoded stream
    static SizeT ComputeDecodedSize(const uchar* srcPtr, SizeT srcNumBytes);
    /// encode byte buffer to RLE stream, returns size of RLE stream
    static SizeT Encode(const uchar* srcPtr, SizeT srcNumBytes, uchar* dstPtr, SizeT dstNumBytes);
    /// decode RLE stream to byte buffer
    static SizeT Decode(const uchar* srcPtr, SizeT srcNumBytes, uchar* dstPtr, SizeT dstNumBytes);
};

} // namespace Util
//------------------------------------------------------------------------------
