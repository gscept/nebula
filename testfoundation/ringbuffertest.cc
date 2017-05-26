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
    this->Verify(rb.Size() == 0);
    this->Verify(rb.Capacity() == 5);
    this->Verify(rb.IsEmpty());

    rb.Add(1);
    this->Verify(rb.Size() == 1);
    this->Verify(!rb.IsEmpty());
    this->Verify(rb.Front() == 1);
    this->Verify(rb.Back() == 1);
    this->Verify(1 == rb[0]);

    rb.Add(2);
    this->Verify(rb.Size() == 2);
    this->Verify(rb.Front() == 1);
    this->Verify(rb.Back() == 2);
    this->Verify(1 == rb[0]);
    this->Verify(2 == rb[1]); 

    rb.Add(3);
    this->Verify(rb.Size() == 3);
    this->Verify(rb.Front() == 1);
    this->Verify(rb.Back() == 3);
    this->Verify(1 == rb[0]);
    this->Verify(2 == rb[1]);
    this->Verify(3 == rb[2]);

    rb.Add(4);
    this->Verify(rb.Size() == 4);
    this->Verify(rb.Front() == 1);
    this->Verify(rb.Back() == 4);
    this->Verify(1 == rb[0]);
    this->Verify(2 == rb[1]);
    this->Verify(3 == rb[2]);
    this->Verify(4 == rb[3]);

    rb.Add(5);
    this->Verify(rb.Size() == 5);
    this->Verify(rb.Front() == 1);
    this->Verify(rb.Back() == 5);
    this->Verify(1 == rb[0]);
    this->Verify(2 == rb[1]);
    this->Verify(3 == rb[2]);
    this->Verify(4 == rb[3]);
    this->Verify(5 == rb[4]);

    rb.Add(6);
    this->Verify(rb.Size() == 5);
    this->Verify(rb.Front() == 2);
    this->Verify(rb.Back() == 6);
    this->Verify(2 == rb[0]);
    this->Verify(3 == rb[1]);
    this->Verify(4 == rb[2]);
    this->Verify(5 == rb[3]);
    this->Verify(6 == rb[4]);

    rb.Add(7);
    this->Verify(rb.Size() == 5);
    this->Verify(rb.Front() == 3);
    this->Verify(rb.Back() == 7);
    this->Verify(3 == rb[0]);
    this->Verify(4 == rb[1]);
    this->Verify(5 == rb[2]);
    this->Verify(6 == rb[3]);
    this->Verify(7 == rb[4]);

    // test copy constructor and assignment operator
    RingBuffer<int> rb1 = rb;
    this->Verify(rb1.Size() == 5);
    this->Verify(rb1.Front() == 3);
    this->Verify(rb1.Back() == 7);
    this->Verify(3 == rb1[0]);
    this->Verify(4 == rb1[1]);
    this->Verify(5 == rb1[2]);
    this->Verify(6 == rb1[3]);
    this->Verify(7 == rb1[4]);
    rb1.Reset();
    this->Verify(rb1.Size() == 0);
    rb1 = rb;
    this->Verify(rb1.Size() == 5);
    this->Verify(rb1.Front() == 3);
    this->Verify(rb1.Back() == 7);
    this->Verify(3 == rb1[0]);
    this->Verify(4 == rb1[1]);
    this->Verify(5 == rb1[2]);
    this->Verify(6 == rb1[3]);
    this->Verify(7 == rb1[4]);
}

} // namespace Test