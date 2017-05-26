//------------------------------------------------------------------------------
//  queuetest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "queuetest.h"
#include "util/queue.h"

namespace Test
{
__ImplementClass(Test::QueueTest, 'QUET', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
QueueTest::Run()
{
    Queue<int> queue0;
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

    Queue<int> queue1 = queue0;
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
}

}; // namespace Test
