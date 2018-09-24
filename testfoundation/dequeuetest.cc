//------------------------------------------------------------------------------
//  dequeuetest.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "dequeuetest.h"
#include "util/dequeue.h"

namespace Test
{
__ImplementClass(Test::DequeueTest, 'DQET', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
DequeueTest::Run()
{
    DeQueue<int> queue0;
    this->Verify(queue0.Size() == 0);
    this->Verify(queue0.IsEmpty());

    queue0.Enqueue(1);
    queue0.Enqueue(2);
    queue0.Enqueue(3);
    this->Verify(queue0.Size() == 3);
    this->Verify(!queue0.IsEmpty());
    this->Verify(queue0.Peek() == 1);
    this->Verify(queue0.Peek() == 1);
    this->Verify(queue0.Contains(3));
    this->Verify(!queue0.Contains(4));
    this->Verify(queue0[0] == 1);
    this->Verify(queue0[1] == 2);
    this->Verify(queue0[2] == 3);

    DeQueue<int> queue1 = queue0;
    this->Verify(queue0 == queue1);
    this->Verify(!(queue0 != queue1));
    this->Verify(queue1.Dequeue() == 1);
    this->Verify(queue1.Size() == 2);
    this->Verify(queue1[0] == 2);
    this->Verify(queue1[1] == 3);
    this->Verify(queue1 != queue0);
    this->Verify(queue1.Dequeue() == 2);
    this->Verify(queue1.Dequeue() == 3);
    this->Verify(queue1.Size() == 0);
    this->Verify(queue1.IsEmpty());

    queue0.Clear();
    this->Verify(queue0.Size() == 0);
    this->Verify(queue0.IsEmpty());

    SizeT capacity = 2* queue0.Capacity();
    for (IndexT i = 0; i < capacity; i++)
    {
        queue0.Enqueue(i);
    }
    this->Verify(queue0.Size() == capacity);
    bool same = true;
    for (IndexT i = 0; i < capacity; i++)
    {
        same &= queue0.Dequeue() == i;
    }
    this->Verify(same);

    capacity = queue0.Capacity() >> 1;

    for (IndexT i = 0; i < capacity; i++)
    {
        queue0.Enqueue(i);
        queue0.Dequeue();
    }
    IndexT limit = 2 * capacity - 1;
    for (IndexT i = 0; i < limit; i++)
    {
        queue0.Enqueue(i);
    }
    same = true;
    for (IndexT i = 0; i < limit; i++)
    {
        same &= queue0[i] == i;
    }
    this->Verify(same);

    queue0.Grow();
    same = true;
    for (IndexT i = 0; i < limit; i++)
    {
        same &= queue0[i] == i;
    }
    this->Verify(same);
}

}; // namespace Test
