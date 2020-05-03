//------------------------------------------------------------------------------
//  stacktest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "stacktest.h"
#include "util/stack.h"

namespace Test
{
__ImplementClass(Test::StackTest, 'STKT', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
StackTest::Run()
{
    Stack<int> stack0;
    VERIFY(stack0.Size() == 0);
    VERIFY(stack0.IsEmpty());
    stack0.Push(1);
    stack0.Push(2);
    stack0.Push(3);
    stack0.Push(4);
    VERIFY(!stack0.IsEmpty());
    VERIFY(stack0.Size() == 4);
    VERIFY(stack0.Peek() == 4);
    VERIFY(stack0.Peek() == 4);
    VERIFY(stack0.Contains(2));
    VERIFY(!stack0.Contains(10));
    VERIFY(stack0[0] == 4);
    VERIFY(stack0[1] == 3);
    VERIFY(stack0[2] == 2);
    VERIFY(stack0[3] == 1);

    Stack<int> stack1 = stack0;
    VERIFY(stack0 == stack1);
    VERIFY(stack1.Pop() == 4);
    VERIFY(stack1.Size() == 3);
    VERIFY(stack1[0] == 3);
    VERIFY(stack1[1] == 2);
    VERIFY(stack1[2] == 1);
    VERIFY(stack0 != stack1);
    VERIFY(stack1.Pop() == 3);
    VERIFY(stack1.Pop() == 2);
    VERIFY(stack1.Pop() == 1);
    VERIFY(stack1.Size() == 0);
    
    stack0.Clear();
    VERIFY(stack0.Size() == 0);
    VERIFY(stack0 == stack1);
}

}; // namespace Test
