//------------------------------------------------------------------------------
//  arrayqueuetest.cc
//  (C) 2006 Radon Labs GmbH
//  (C)2013 - 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "arrayqueuetest.h"
#include "util/arrayqueue.h"

namespace Test
{
__ImplementClass(Test::ArrayQueueTest, 'QUET', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
ArrayQueueTest::Run()
{
    ArrayQueue<int> queue0;
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

    ArrayQueue<int> queue1 = queue0;
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
