//------------------------------------------------------------------------------
//  ringbuffertest.cc
//  (C) 2008 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "ringbuffertest.h"
#include "util/ringbuffer.h"

namespace Test
{
__ImplementClass(Test::RingBufferTest, 'RBTT', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
RingBufferTest::Run()
{
    RingBuffer<int> rb(5);
    VERIFY(rb.Size() == 0);
    VERIFY(rb.Capacity() == 5);
    VERIFY(rb.IsEmpty());

    rb.Add(1);
    VERIFY(rb.Size() == 1);
    VERIFY(!rb.IsEmpty());
    VERIFY(rb.Front() == 1);
    VERIFY(rb.Back() == 1);
    VERIFY(1 == rb[0]);

    rb.Add(2);
    VERIFY(rb.Size() == 2);
    VERIFY(rb.Front() == 1);
    VERIFY(rb.Back() == 2);
    VERIFY(1 == rb[0]);
    VERIFY(2 == rb[1]); 

    rb.Add(3);
    VERIFY(rb.Size() == 3);
    VERIFY(rb.Front() == 1);
    VERIFY(rb.Back() == 3);
    VERIFY(1 == rb[0]);
    VERIFY(2 == rb[1]);
    VERIFY(3 == rb[2]);

    rb.Add(4);
    VERIFY(rb.Size() == 4);
    VERIFY(rb.Front() == 1);
    VERIFY(rb.Back() == 4);
    VERIFY(1 == rb[0]);
    VERIFY(2 == rb[1]);
    VERIFY(3 == rb[2]);
    VERIFY(4 == rb[3]);

    rb.Add(5);
    VERIFY(rb.Size() == 5);
    VERIFY(rb.Front() == 1);
    VERIFY(rb.Back() == 5);
    VERIFY(1 == rb[0]);
    VERIFY(2 == rb[1]);
    VERIFY(3 == rb[2]);
    VERIFY(4 == rb[3]);
    VERIFY(5 == rb[4]);

    rb.Add(6);
    VERIFY(rb.Size() == 5);
    VERIFY(rb.Front() == 2);
    VERIFY(rb.Back() == 6);
    VERIFY(2 == rb[0]);
    VERIFY(3 == rb[1]);
    VERIFY(4 == rb[2]);
    VERIFY(5 == rb[3]);
    VERIFY(6 == rb[4]);

    rb.Add(7);
    VERIFY(rb.Size() == 5);
    VERIFY(rb.Front() == 3);
    VERIFY(rb.Back() == 7);
    VERIFY(3 == rb[0]);
    VERIFY(4 == rb[1]);
    VERIFY(5 == rb[2]);
    VERIFY(6 == rb[3]);
    VERIFY(7 == rb[4]);

    // test copy constructor and assignment operator
    RingBuffer<int> rb1 = rb;
    VERIFY(rb1.Size() == 5);
    VERIFY(rb1.Front() == 3);
    VERIFY(rb1.Back() == 7);
    VERIFY(3 == rb1[0]);
    VERIFY(4 == rb1[1]);
    VERIFY(5 == rb1[2]);
    VERIFY(6 == rb1[3]);
    VERIFY(7 == rb1[4]);
    rb1.Reset();
    VERIFY(rb1.Size() == 0);
    rb1 = rb;
    VERIFY(rb1.Size() == 5);
    VERIFY(rb1.Front() == 3);
    VERIFY(rb1.Back() == 7);
    VERIFY(3 == rb1[0]);
    VERIFY(4 == rb1[1]);
    VERIFY(5 == rb1[2]);
    VERIFY(6 == rb1[3]);
    VERIFY(7 == rb1[4]);
}

} // namespace Test