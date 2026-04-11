//------------------------------------------------------------------------------
//  queuetest.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "queuetest.h"
#include "util/queue.h"
#include "util/arrayqueue.h"
#include <functional>

namespace Test
{
__ImplementClass(Test::QueueTest, 'DQET', Test::TestCase);

using namespace Util;

bool Compare(Util::Queue<int> const & q, Util::ArrayQueue<int> const & a)
{
    bool same = true;
    if (a.Size() != q.Size())
    {
        return false;
    }
    for (int i = 0; i < q.Size(); i++)
    {
        same &= a[i] == q[i];
    }
    return same;
}

struct TestStruct
{
    Util::String str;
    int i;
    std::function<int()> func;
    const TestStruct& operator=(const TestStruct& other)
    {
        str = other.str;
        i = other.i;
        func = other.func;
        return *this;
    }
    TestStruct(const Util::String& s, int i, std::function<int()> f) : str(s), i(i), func(f) { initCount++; }
    TestStruct(const TestStruct& other) : str(other.str), i(other.i), func(other.func) { initCount++; }
    TestStruct() { initCount++; }
    ~TestStruct() { initCount--; }
    static int initCount;
};
int TestStruct::initCount = 0;
//------------------------------------------------------------------------------
/**
*/
void
QueueTest::Run()
{
    Queue<int> queue0;
    VERIFY(queue0.Size() == 0);
    VERIFY(queue0.IsEmpty());

    queue0.Enqueue(1);
    queue0.Enqueue(2);
    queue0.Enqueue(3);
    VERIFY(queue0.Size() == 3);
    VERIFY(!queue0.IsEmpty());
    VERIFY(queue0.Peek() == 1);
    VERIFY(queue0.Peek() == 1);
    VERIFY(queue0.Contains(3));
    VERIFY(!queue0.Contains(4));
    VERIFY(queue0[0] == 1);
    VERIFY(queue0[1] == 2);
    VERIFY(queue0[2] == 3);    

    Queue<int> queue1 = queue0;
    VERIFY(queue0 == queue1);
    VERIFY(!(queue0 != queue1));
    VERIFY(queue1.Dequeue() == 1);
    VERIFY(queue1.Size() == 2);
    VERIFY(queue1[0] == 2);
    VERIFY(queue1[1] == 3);
    VERIFY(queue1 != queue0);
    VERIFY(queue1.Dequeue() == 2);
    VERIFY(queue1.Dequeue() == 3);
    VERIFY(queue1.Size() == 0);
    VERIFY(queue1.IsEmpty());

    queue0.EraseIndex(1);
    VERIFY(queue0.Size() == 2);
    VERIFY(queue0[0] == 1);
    VERIFY(queue0[1] == 3);    

    queue0.Clear();
    VERIFY(queue0.Size() == 0);
    VERIFY(queue0.IsEmpty());

    SizeT capacity = 2* queue0.Capacity();
    for (IndexT i = 0; i < capacity; i++)
    {
        queue0.Enqueue(i);
    }
    VERIFY(queue0.Size() == capacity);
    bool same = true;
    for (IndexT i = 0; i < capacity; i++)
    {
        same &= queue0.Dequeue() == i;
    }
    VERIFY(same);

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
    VERIFY(same);

    queue0.Grow();
    same = true;
    for (IndexT i = 0; i < limit; i++)
    {
        same &= queue0[i] == i;
    }
    VERIFY(same);

    queue0.Clear();
    queue1.Clear();

    Queue<int> q0;
    Queue<int> q1;
    ArrayQueue<int> aq;
    IndexT count = 16;
    IndexT half = count >> 1;
    for (IndexT i = 0; i < half + (half>>1); i++)
    {
        q0.Enqueue(i);
        aq.Enqueue(i);
    }
    
    for (IndexT i = 0; i < half; i++)
    {
        q0.Enqueue(i);
        q0.Dequeue();
        aq.Enqueue(i);
        aq.Dequeue();
    }
    q1 = q0;

    // q0 i having a wraparound, q1 is linear in memory from index 0
    VERIFY(q0 == q1);
    VERIFY(Compare(q0, aq));
    q0.EraseIndex(2);
    q1.EraseIndex(2);
    aq.EraseIndex(2);
    VERIFY(q0 == q1);    
    q0.EraseIndex(q0.Size() - 2);
    q1.EraseIndex(q1.Size() - 2);
    aq.EraseIndex(aq.Size() - 2);
    VERIFY(q0 == q1);
    VERIFY(Compare(q0, aq));    
    q0.EraseIndex(q0.Size() - 1);
    q1.EraseIndex(q1.Size() - 1);
    aq.EraseIndex(aq.Size() - 1);
    VERIFY(q0 == q1);
    VERIFY(Compare(q0, aq));

    // non POD tests
    {
        Queue<Util::String> q;
        ArrayQueue<Util::String> aq;
        q.Enqueue("Hello");
        q.Enqueue("World");
        q.Reserve(q.Capacity() + 10);
        VERIFY(q.Peek() == "Hello");
        VERIFY(q.Dequeue() == "Hello");
        VERIFY(q.Dequeue() == "World");
        VERIFY(q.IsEmpty());

        q.Enqueue("Hello");
        q.Enqueue("World");
        q.Enqueue("World2");
        q.Enqueue("World3");
        q.EraseIndex(0);
        VERIFY(q.Peek() == "World");
        q.EraseIndex(1);
        VERIFY(q.Peek() == "World");
        VERIFY(q[1] == "World3");
        q.EraseIndex(0);
        VERIFY(q.Dequeue() == "World3");
        VERIFY(q.IsEmpty());

        
        Queue<TestStruct> q2;
        {
            TestStruct ts1;
            ts1.str = "Hello";
            ts1.i = 42;
            ts1.func = []() { return 42; };
            q2.Enqueue({ "Hello", 42, []() { return 45; } });
            q2.Enqueue(ts1);
            q2.Enqueue(ts1);
            q2.Reserve(q2.Capacity() + 10);
            {
                [[maybe_unused]] char stackbuffer[256];
                TestStruct ts;
                ts.str = "World";
                ts.i = 24;
                Util::String closureStr = "Closure";
                ts.func = [closureStr]() { return closureStr.Length(); };
                q2.Enqueue(std::move(ts));
            }
            q2.EraseIndex(1);
            VERIFY(q2.Dequeue().func() == 45);
            VERIFY(q2.Dequeue().func() == 42);
            VERIFY(q2.Dequeue().func() == 7);
            VERIFY(q2.IsEmpty());
        }
    }
    VERIFY(TestStruct::initCount == 0);

    {
        struct DestructorCounter
        {
            ~DestructorCounter() { value = -1; }
            int value;
        };
        static const auto MakeSplitQueue = []() -> Queue<DestructorCounter>
        {
            Queue<DestructorCounter> q;
            q.Grow();
            const int capacity = q.Capacity();
            const int half = capacity >> 1;
            for (int i = 0; i< capacity; i++)
            {
                q.Enqueue({i});
            }
            for (int i = 0; i < half; i++)
            {
                q.Dequeue();      
            }
            for(int i = 0; i < half; i++)
            {
                q.Enqueue({i + capacity});
            }
            return q;
        };

        Queue<DestructorCounter> q = MakeSplitQueue();
        const int capacity = q.Capacity();
        const int half = capacity >> 1;
        // we now have a wraparound, and the first half of the queue is in the middle of the buffer,
        // and the second half is at the beginning of the buffer. We erase in the second half, which 
        //forces the elements in the first half to be move-constructed into the second half, and then 
        // we verify that the remaining elements are correct.
        q.EraseIndex(capacity * 3 / 4);
        same = true;
        for (int i = 0 ; i < q.Size(); i++)
        {
            int offset = i < capacity * 3 / 4 ? half : half + 1;
            same &= q[i].value == (i + offset);
        }
        VERIFY(same);

        Queue<DestructorCounter> q2 = MakeSplitQueue();
        q2.EraseIndex(capacity/4);
        bool allValid = true;
        for (int i = 0, k = q2.Size(); i < k; i++)   
        {
            allValid &= q2.Dequeue().value != -1;
        }
        VERIFY(allValid);
    }
}

}; // namespace Test
