//------------------------------------------------------------------------------
//  listtest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "listtest.h"
#include "util/list.h"
#include "util/string.h"

namespace Test
{
__ImplementClass(Test::ListTest, 'LSTT', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
ListTest::Run()
{
    List<String> list;
    this->Verify(list.Size() == 0);
    this->Verify(list.IsEmpty());

    list.AddFront("Anne");
    list.AddFront("Berit");
    list.AddFront("Conni");
    this->Verify(!list.IsEmpty());
    this->Verify(list.Size() == 3);
    this->Verify(list.Front() == "Conni");
    this->Verify(list.Back() == "Anne");
    list.RemoveFront();
    this->Verify(list.Size() == 2);
    this->Verify(list.Front() == "Berit");
    list.RemoveFront();
    this->Verify(list.Size() == 1);
    this->Verify(list.Front() == "Anne");
    this->Verify(list.Back() == "Anne");
    list.RemoveFront();
    this->Verify(list.Size() == 0);

    list.AddBack("Anne");
    list.AddBack("Berit");
    list.AddBack("Conni");
    this->Verify(list.Size() == 3);
    this->Verify(list.Front() == "Anne");
    this->Verify(list.Back() == "Conni");
    list.RemoveBack();
    this->Verify(list.Size() == 2);
    this->Verify(list.Back() == "Berit");
    list.RemoveBack();
    this->Verify(list.Size() == 1);
    this->Verify(list.Back() == "Anne");
    list.RemoveBack();
    this->Verify(list.Size() == 0);

    list.AddBack("Anne");
    list.AddBack("Danni");
    List<String>::Iterator iter = list.Find("Danni", list.Begin());
    n_assert(0 != iter);
    list.AddBefore(iter, "Conni");
    iter = list.Find("Anne", list.Begin());
    n_assert(0 != iter);
    list.AddAfter(iter, "Berit");
    iter = list.Find("Angelika", list.Begin());
    this->Verify(0 == iter);
    this->Verify(list.Size() == 4);
    this->Verify(list.Front() == "Anne");
    this->Verify(list.Back() == "Danni");
    list.RemoveFront();
    list.RemoveBack();
    this->Verify(list.Size() == 2);
    this->Verify(list.Front() == "Berit");
    this->Verify(list.Back() == "Conni");
    list.Clear();
    this->Verify(list.Size() == 0);
}

}; // namespace Test

