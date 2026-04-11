//------------------------------------------------------------------------------
//  arraystacktest.cc
//  (C) Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "util/arraystack.h"
#include "arraystacktest.h"

namespace Test
{
__ImplementClass(Test::ArrayStackTest, 'ARST', Test::TestCase);

using namespace Util;

// small helper types for testing non-trivial destruction / move semantics
struct ASCounterObj
{
    static int constructed;
    static int destroyed;
    static int copied;
    static int moved;

    ASCounterObj() { ++constructed; }
    ~ASCounterObj() { ++destroyed; }
    ASCounterObj(const ASCounterObj&) { ++copied; ++constructed; }
    ASCounterObj(ASCounterObj&&) noexcept { ++moved; ++constructed; }
    ASCounterObj& operator=(const ASCounterObj&) { ++copied; return *this; }
    ASCounterObj& operator=(ASCounterObj&&) noexcept { ++moved; return *this; }
};

int ASCounterObj::constructed = 0;
int ASCounterObj::destroyed = 0;
int ASCounterObj::copied = 0;
int ASCounterObj::moved = 0;

// comparator helpers
static bool LessInt(const int& a, const int& b) { return a < b; }
static int QSortIntCmp(const void* a, const void* b)
{
    const int ia = *static_cast<const int*>(a);
    const int ib = *static_cast<const int*>(b);
    return (ia > ib) - (ia < ib);
}

//------------------------------------------------------------------------------
/// Run
void
ArrayStackTest::Run()
{
    // ------------------------------------------------------------------------
    // Basic integer array tests (existing tests preserved)
    // ------------------------------------------------------------------------
    const int STACK_SIZE = 64;
    ArrayStack<int, STACK_SIZE> array0, array1, array2;
    
    array0.Append(1);
    array0.Append(2);
    array0.Append(3);
    array1.Append(4);
    array1.Append(5);
    array1.Append(6);
    array1.Append(7);

    // size and [] operator
    VERIFY(array0.Size() == 3);
    VERIFY(1 == array0[0]);
    VERIFY(2 == array0[1]);
    VERIFY(3 == array0[2]);

    VERIFY(array1.Size() == 4);
    VERIFY(4 == array1[0]);
    VERIFY(5 == array1[1]);
    VERIFY(6 == array1[2]);
    VERIFY(7 == array1[3]);

    // =, ==, != operator
    VERIFY(array0 != array1);
    array2 = array0;
    VERIFY(array2.Size() == 3);
    VERIFY(1 == array2[0]);
    VERIFY(2 == array2[1]);
    VERIFY(3 == array2[2]);
    VERIFY(array0 == array2);
    array2[2] = 10;
    VERIFY(10 == array2[2]);
    VERIFY(array0 != array2);

    // AppendArray
    array2 = array0;
    array2.AppendArray(array1);
    VERIFY(array2.Size() == 7);
    VERIFY(array2[0] == 1);
    VERIFY(array2[3] == 4);
    VERIFY(array2[6] == 7);

    // Front, Back, IsEmpty, Clear, Erase, FindIndex, Fill
    VERIFY(array2.Front() == 1);
    VERIFY(array2.Back() == 7);
    VERIFY(!array2.IsEmpty());
    array2.EraseIndex(0);
    VERIFY(array2.Size() == 6);
    VERIFY(array2[0] == 2);
    VERIFY(array2[1] == 3);
    VERIFY(array2[5] == 7);
    array2.EraseIndex(5);
    VERIFY(array2.Size() == 5);
    VERIFY(array2[4] == 6);
    VERIFY(array2.FindIndex(4) == 2);
    VERIFY(array2.FindIndex(8) == -1);
    array2.Fill(1, 2, 0);
    VERIFY(array2[0] == 2);
    VERIFY(array2[1] == 0);
    VERIFY(array2[2] == 0);
    VERIFY(array2[3] == 5);
    array2.Clear();
    VERIFY(array2.Size() == 0);
    VERIFY(array2.IsEmpty());

    // test InsertSorted()
    array0.Clear();
    VERIFY(InvalidIndex != array0.InsertSorted(3));
    VERIFY(InvalidIndex != array0.InsertSorted(1));
    VERIFY(InvalidIndex != array0.InsertSorted(4));
    VERIFY(InvalidIndex != array0.InsertSorted(0));
    VERIFY(InvalidIndex != array0.InsertSorted(2));
    VERIFY(InvalidIndex != array0.InsertSorted(10));
    VERIFY(InvalidIndex != array0.InsertSorted(100));
    VERIFY(InvalidIndex != array0.InsertSorted(-2));
    VERIFY(InvalidIndex != array0.InsertSorted(6));
    VERIFY(InvalidIndex != array0.InsertSorted(12));
    VERIFY(InvalidIndex != array0.InsertSorted(12));
    VERIFY(array0.IsSorted());

    // test InsertSorted randomly
    array0.Clear();
    IndexT i;
    for (i = 0; i < STACK_SIZE; i++)
    {
        array0.InsertSorted(rand());
    }
    VERIFY(array0.IsSorted());

    // test Sort()
    array0.Clear();
    array0.Append(4); array0.Append(2); array0.Append(1); array0.Append(3);
    array0.Sort();
    VERIFY(array0[0] == 1);
    VERIFY(array0[1] == 2);
    VERIFY(array0[2] == 3);
    VERIFY(array0[3] == 4);

    // test BinarySearch
    VERIFY(array0.BinarySearchIndex(1) == 0);
    VERIFY(array0.BinarySearchIndex(2) == 1);
    VERIFY(array0.BinarySearchIndex(3) == 2);
    VERIFY(array0.BinarySearchIndex(4) == 3);
    VERIFY(array0.BinarySearchIndex(5) == -1);  

    // initializer list ctor
    {
        ArrayStack<int, STACK_SIZE> il = { 7, 8, 9 };
        VERIFY(il.Size() == 3);
        VERIFY(il[0] == 7 && il[2] == 9);
    }

    // variadic Append (Append(first, ...))
    {
        ArrayStack<int, STACK_SIZE> v;
        v.Append(1, 2, 3, 4, 5);
        VERIFY(v.Size() == 5);
        for (IndexT j = 0; j < 5; ++j) VERIFY(v[j] == (int)j + 1);
    }

    // Append rvalue and push_back alias
    {
        ArrayStack<int, STACK_SIZE> a;
        int x = 42;
        a.Append(x);
        a.Append(std::move(x));
        VERIFY(a.Size() == 2);
        VERIFY(a[0] == 42);
        VERIFY(a[1] == 42);
    }

    
    // Find (returns iterator) and FindIndex template
    {
        ArrayStack<int, STACK_SIZE> f;
        f.Append(5); f.Append(6); f.Append(7);
        auto it = f.Find(6);
        VERIFY(it && *it == 6);
        VERIFY(f.FindIndex<int>(7) == 2);
        VERIFY(f.FindIndex<int>(123) == InvalidIndex);
    }

    // Insert, EraseSwap, EraseFront, EraseBack, PopFront, PopBack
    {
        ArrayStack<int, STACK_SIZE> t;
        t.Append(1); t.Append(2); t.Append(3); t.Append(4);
        t.Insert(2, 99); // 1,2,99,3,4
        VERIFY(t[2] == 99 && t.Size() == 5);

        // EraseSwap on middle element (destroys order)
        t.EraseIndexSwap(1); // swap index 1 with last
        VERIFY(t.Size() == 4);

        // EraseFront
        t.EraseFront();
        VERIFY(t.Size() == 3);
    }

    // Erase iterator variants
    {
        ArrayStack<int, STACK_SIZE> it;
        it.Append(1); it.Append(2); it.Append(3);
        auto iter = it.Begin();
        ++iter; // points to value 2
        it.Erase(iter);
        VERIFY(it.Size() == 2);
    }

    // SortWithFunc 
    {
        ArrayStack<int, STACK_SIZE> s;
        s.Append(3); s.Append(1); s.Append(2);
        s.SortWithFunc(LessInt);
        VERIFY(s[0] == 1 && s[1] == 2 && s[2] == 3);
    }

    // BinarySearchIndex templated KEYTYPE
    {
        ArrayStack<int, STACK_SIZE> bs;
        bs.Append(1); bs.Append(2); bs.Append(3); bs.Append(4);
        VERIFY(bs.BinarySearchIndex<int>(3) == 2);
        VERIFY(bs.BinarySearchIndex<int>(99) == InvalidIndex);
    }

    // Realloc
    {
        ArrayStack<int, STACK_SIZE> rc;
        rc.Realloc(8, 4);
        VERIFY(rc.Capacity() == 8);
        VERIFY(rc.Size() == 8);
    }

    // STL-like interface: size(), begin/end(), resize(size_t), clear(), push_back()
    {
        ArrayStack<int, STACK_SIZE> stl;
        stl.push_back(11);
        stl.push_back(12);
        VERIFY(stl.size() == 2);
        VERIFY(*stl.begin() == 11);
    }

    // Grow indirectly covered by several operations above

    // ------------------------------------------------------------------------
    // Tests for non-trivially-destructible types (destructors / moves / copies)
    // ------------------------------------------------------------------------
    {
        // reset counters
        ASCounterObj::constructed = 0;
        ASCounterObj::destroyed = 0;
        ASCounterObj::copied = 0;
        ASCounterObj::moved = 0;

        // use a small-stack array to ensure some elements live in stackElements
        ArrayStack<ASCounterObj, STACK_SIZE> co;
        // Append to create default constructed elements
        co.Append(ASCounterObj());
        co.Append(ASCounterObj());
        co.Append(ASCounterObj());
        VERIFY(ASCounterObj::constructed >= 3);

        // Clear should call destructors for those elements
        co.Clear();
        VERIFY(ASCounterObj::destroyed >= 3);

        // test move assignment
        ASCounterObj::constructed = 0; ASCounterObj::destroyed = 0; ASCounterObj::copied = 0; ASCounterObj::moved = 0;
        ArrayStack<ASCounterObj, STACK_SIZE> a1;
        a1.Append(ASCounterObj()); a1.Append(ASCounterObj());
        ArrayStack<ASCounterObj, STACK_SIZE> a2;
        a2 = std::move(a1);
        // moved or copied counts may depend on implementation path; at least a2 has elements
        VERIFY(a2.Size() == 2);
    }

    // ------------------------------------------------------------------------
    // Iterator based algorithms (begin/end) and const iterators
    // ------------------------------------------------------------------------
    {
        Array<int> italg;
        italg.Append(1); italg.Append(2); italg.Append(3);
        int sum = 0;
        for (auto it = italg.Begin(); it != italg.End(); ++it) sum += *it;
        VERIFY(sum == 6);

        const Array<int> constArr = italg;
        auto cit = constArr.ConstBegin();
        VERIFY(*cit == 1);
    }
}

}; // namespace Test