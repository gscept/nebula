//------------------------------------------------------------------------------
//  runlengthcodec.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/runlengthcodec.h"

namespace Util
{

//------------------------------------------------------------------------------
/**
    Get some safe destination buffer size for runlength-encoding.
    This actually can be up to twice as big as the source buffer for
    completely random data!
*/
SizeT
RunLengthCodec::GetSafeRLEBufferSize(SizeT srcBufferSize)
{
    return srcBufferSize * 2;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
RunLengthCodec::Encode(const uchar* srcPtr, SizeT srcNumBytes, uchar* dstPtr, SizeT dstNumBytes)
{
    n_assert(srcNumBytes >= 2);
    n_assert(0 != srcPtr);
    n_assert(0 != dstPtr);
    n_assert((srcNumBytes > 0) && (dstNumBytes >= RunLengthCodec::GetSafeRLEBufferSize(srcNumBytes)));

    const uchar* dstStore = dstPtr;
    const uchar* srcEndPtr = srcPtr + srcNumBytes;

    while (srcPtr < srcEndPtr)
    {
        uchar curByte = *srcPtr++;
        SizeT numBytes = 1;
        while ((srcPtr < srcEndPtr) && (curByte == *srcPtr) && (numBytes < 255))
        {
            numBytes++;
            srcPtr++;
        }
        *dstPtr++ = (uchar) numBytes;
        *dstPtr++ = curByte;
    }
    return SizeT(dstPtr - dstStore);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
RunLengthCodec::ComputeDecodedSize(const uchar* srcPtr, SizeT srcNumBytes)
{
    SizeT decodedSize = 0;
    const uchar* srcEndPtr = srcPtr + srcNumBytes;
    while (srcPtr < srcEndPtr)
    {
        uchar num = *srcPtr++;
        srcPtr++;
        decodedSize += num;
    }
    return decodedSize;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
RunLengthCodec::Decode(const uchar* srcPtr, SizeT srcNumBytes, uchar* dstPtr, SizeT dstNumBytes)
{
    n_assert(srcNumBytes >= 2);
    n_assert((srcNumBytes & 1) == 0);

    const uchar* srcEndPtr = srcPtr + srcNumBytes;
    const uchar* dstEndPtr = dstPtr + dstNumBytes;
    const uchar* dstStore = dstPtr;
    
    while (srcPtr < srcEndPtr)
    {
        uchar num = *srcPtr++;
        uchar val = *srcPtr++;
        IndexT i;
        for (i = 0; i < num; i++)
        {
            n_assert(dstPtr < dstEndPtr);
            *dstPtr++ = val;
        }
    }
    return SizeT(dstPtr - dstStore);
}

} // namespace Util