//------------------------------------------------------------------------------
//  runlengthcodectest.cc
//  (C) 2008 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "runlengthcodectest.h"
#include "util/runlengthcodec.h"

namespace Test
{
__ImplementClass(Test::RunLengthCodecTest, 'RLET', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
RunLengthCodecTest::Run()
{
    // test a simple small string
    uchar srcData[] = "aaaaaabcddccc";
    uchar rleData[32];
    uchar decodeData[32];

    SizeT rleSize = RunLengthCodec::Encode(srcData, 13, rleData, sizeof(rleData));
    VERIFY(10 == rleSize);
    VERIFY(rleData[0] == 6);
    VERIFY(rleData[1] == 'a');
    VERIFY(rleData[2] == 1);
    VERIFY(rleData[3] == 'b');
    VERIFY(rleData[4] == 1);
    VERIFY(rleData[5] == 'c');
    VERIFY(rleData[6] == 2);
    VERIFY(rleData[7] == 'd');
    VERIFY(rleData[8] == 3);
    VERIFY(rleData[9] == 'c');
    VERIFY(RunLengthCodec::ComputeDecodedSize(rleData, rleSize) == 13);

    SizeT decodeSize = RunLengthCodec::Decode(rleData, rleSize, decodeData, sizeof(decodeData));
    VERIFY(decodeSize == 13);
    VERIFY(decodeData[0] == 'a');
    VERIFY(decodeData[1] == 'a');
    VERIFY(decodeData[2] == 'a');
    VERIFY(decodeData[3] == 'a');
    VERIFY(decodeData[4] == 'a');
    VERIFY(decodeData[5] == 'a');
    VERIFY(decodeData[6] == 'b');
    VERIFY(decodeData[7] == 'c');
    VERIFY(decodeData[8] == 'd');
    VERIFY(decodeData[9] == 'd');
    VERIFY(decodeData[10] == 'c');
    VERIFY(decodeData[11] == 'c');
    VERIFY(decodeData[12] == 'c');

    // test some large data with many identical pieces
    const SizeT bufSize = 2001;
    SizeT rleBufferSize = RunLengthCodec::GetSafeRLEBufferSize(bufSize);
    uchar* srcBuffer = (uchar*) Memory::Alloc(Memory::ScratchHeap, rleBufferSize);
    Memory::Clear(srcBuffer, bufSize);
    uchar* rleBuffer = (uchar*) Memory::Alloc(Memory::ScratchHeap, rleBufferSize);
    Memory::Clear(rleBuffer, rleBufferSize);
    uchar* dstBuffer = (uchar*) Memory::Alloc(Memory::ScratchHeap, bufSize);
    Memory::Clear(dstBuffer, bufSize);

    srcBuffer[197] = 1;
    srcBuffer[1111] = 1;
    srcBuffer[1993] = 1;
    srcBuffer[2000] = 1;

    rleSize = RunLengthCodec::Encode(srcBuffer, bufSize, rleBuffer, rleBufferSize);
    VERIFY(RunLengthCodec::ComputeDecodedSize(rleBuffer, rleSize) == 2001);
    decodeSize = RunLengthCodec::Decode(rleBuffer, rleSize, dstBuffer, bufSize);
    VERIFY(decodeSize == bufSize);
    bool isIdentical = true;
    IndexT i;
    for (i = 0; i < bufSize; i++)
    {
        if (srcBuffer[i] != dstBuffer[i])
        {
            isIdentical = false;
            break;
        }
    }
    VERIFY(isIdentical);

    // test some semi-random data
    for (i = 0; i < bufSize; i++)
    {
        srcBuffer[i] = (uchar) (rand() % 16);
    }
    rleSize = RunLengthCodec::Encode(srcBuffer, bufSize, rleBuffer, rleBufferSize);
    VERIFY(RunLengthCodec::ComputeDecodedSize(rleBuffer, rleSize) == 2001);
    decodeSize = RunLengthCodec::Decode(rleBuffer, rleSize, dstBuffer, bufSize);
    VERIFY(decodeSize == bufSize);
    isIdentical = true;
    for (i = 0; i < bufSize; i++)
    {
        if (srcBuffer[i] != dstBuffer[i])
        {
            isIdentical = false;
            break;
        }
    }
    VERIFY(isIdentical);

    Memory::Free(Memory::ScratchHeap, srcBuffer);
    Memory::Free(Memory::ScratchHeap, rleBuffer);
    Memory::Free(Memory::ScratchHeap, dstBuffer);
}

} // namespace Test

