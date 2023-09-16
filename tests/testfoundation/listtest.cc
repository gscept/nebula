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
    VERIFY(list.Size() == 0);
    VERIFY(list.IsEmpty());

    list.AddFront("Anne");
    list.AddFront("Berit");
    list.AddFront("Conni");
    VERIFY(!list.IsEmpty());
    VERIFY(list.Size() == 3);
    VERIFY(list.Front() == "Conni");
    VERIFY(list.Back() == "Anne");
    list.RemoveFront();
    VERIFY(list.Size() == 2);
    VERIFY(list.Front() == "Berit");
    list.RemoveFront();
    VERIFY(list.Size() == 1);
    VERIFY(list.Front() == "Anne");
    VERIFY(list.Back() == "Anne");
    list.RemoveFront();
    VERIFY(list.Size() == 0);

    list.AddBack("Anne");
    list.AddBack("Berit");
    list.AddBack("Conni");
    VERIFY(list.Size() == 3);
    VERIFY(list.Front() == "Anne");
    VERIFY(list.Back() == "Conni");
    list.RemoveBack();
    VERIFY(list.Size() == 2);
    VERIFY(list.Back() == "Berit");
    list.RemoveBack();
    VERIFY(list.Size() == 1);
    VERIFY(list.Back() == "Anne");
    list.RemoveBack();
    VERIFY(list.Size() == 0);

    list.AddBack("Anne");
    list.AddBack("Danni");
    List<String>::Iterator iter = list.Find("Danni", list.Begin());
    n_assert(nullptr != iter);
    list.AddBefore(iter, "Conni");
    iter = list.Find("Anne", list.Begin());
    n_assert(nullptr != iter);
    list.AddAfter(iter, "Berit");
    iter = list.Find("Angelika", list.Begin());
    VERIFY(nullptr == iter);
    VERIFY(list.Size() == 4);
    VERIFY(list.Front() == "Anne");
    VERIFY(list.Back() == "Danni");
    list.RemoveFront();
    list.RemoveBack();
    VERIFY(list.Size() == 2);
    VERIFY(list.Front() == "Berit");
    VERIFY(list.Back() == "Conni");
    list.Clear();
    VERIFY(list.Size() == 0);
}

}; // namespace Test

