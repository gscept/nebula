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
    this->Verify(10 == rleSize);
    this->Verify(rleData[0] == 6);
    this->Verify(rleData[1] == 'a');
    this->Verify(rleData[2] == 1);
    this->Verify(rleData[3] == 'b');
    this->Verify(rleData[4] == 1);
    this->Verify(rleData[5] == 'c');
    this->Verify(rleData[6] == 2);
    this->Verify(rleData[7] == 'd');
    this->Verify(rleData[8] == 3);
    this->Verify(rleData[9] == 'c');
    this->Verify(RunLengthCodec::ComputeDecodedSize(rleData, rleSize) == 13);

    SizeT decodeSize = RunLengthCodec::Decode(rleData, rleSize, decodeData, sizeof(decodeData));
    this->Verify(decodeSize == 13);
    this->Verify(decodeData[0] == 'a');
    this->Verify(decodeData[1] == 'a');
    this->Verify(decodeData[2] == 'a');
    this->Verify(decodeData[3] == 'a');
    this->Verify(decodeData[4] == 'a');
    this->Verify(decodeData[5] == 'a');
    this->Verify(decodeData[6] == 'b');
    this->Verify(decodeData[7] == 'c');
    this->Verify(decodeData[8] == 'd');
    this->Verify(decodeData[9] == 'd');
    this->Verify(decodeData[10] == 'c');
    this->Verify(decodeData[11] == 'c');
    this->Verify(decodeData[12] == 'c');

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
    this->Verify(RunLengthCodec::ComputeDecodedSize(rleBuffer, rleSize) == 2001);
    decodeSize = RunLengthCodec::Decode(rleBuffer, rleSize, dstBuffer, bufSize);
    this->Verify(decodeSize == bufSize);
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
    this->Verify(isIdentical);

    // test some semi-random data
    for (i = 0; i < bufSize; i++)
    {
        srcBuffer[i] = (uchar) (rand() % 16);
    }
    rleSize = RunLengthCodec::Encode(srcBuffer, bufSize, rleBuffer, rleBufferSize);
    this->Verify(RunLengthCodec::ComputeDecodedSize(rleBuffer, rleSize) == 2001);
    decodeSize = RunLengthCodec::Decode(rleBuffer, rleSize, dstBuffer, bufSize);
    this->Verify(decodeSize == bufSize);
    isIdentical = true;
    for (i = 0; i < bufSize; i++)
    {
        if (srcBuffer[i] != dstBuffer[i])
        {
            isIdentical = false;
            break;
        }
    }
    this->Verify(isIdentical);

    Memory::Free(Memory::ScratchHeap, srcBuffer);
    Memory::Free(Memory::ScratchHeap, rleBuffer);
    Memory::Free(Memory::ScratchHeap, dstBuffer);
}

} // namespace Test

