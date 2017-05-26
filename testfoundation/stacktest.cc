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
    this->Verify(stack0.Size() == 0);
    this->Verify(stack0.IsEmpty());
    stack0.Push(1);
    stack0.Push(2);
    stack0.Push(3);
    stack0.Push(4);
    this->Verify(!stack0.IsEmpty());
    this->Verify(stack0.Size() == 4);
    this->Verify(stack0.Peek() == 4);
    this->Verify(stack0.Peek() == 4);
    this->Verify(stack0.Contains(2));
    this->Verify(!stack0.Contains(10));
    this->Verify(stack0[0] == 4);
    this->Verify(stack0[1] == 3);
    this->Verify(stack0[2] == 2);
    this->Verify(stack0[3] == 1);

    Stack<int> stack1 = stack0;
    this->Verify(stack0 == stack1);
    this->Verify(stack1.Pop() == 4);
    this->Verify(stack1.Size() == 3);
    this->Verify(stack1[0] == 3);
    this->Verify(stack1[1] == 2);
    this->Verify(stack1[2] == 1);
    this->Verify(stack0 != stack1);
    this->Verify(stack1.Pop() == 3);
    this->Verify(stack1.Pop() == 2);
    this->Verify(stack1.Pop() == 1);
    this->Verify(stack1.Size() == 0);
    
    stack0.Clear();
    this->Verify(stack0.Size() == 0);
    this->Verify(stack0 == stack1);
}

}; // namespace Test
