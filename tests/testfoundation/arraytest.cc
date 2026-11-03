//------------------------------------------------------------------------------
//  arraytest.cc
//  (C) 2006 Radon Labs GmbH
//  (C) Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "arraytest.h"

namespace Test
{
__ImplementClass(Test::ArrayTest, 'ARRT', Test::TestCase);

using namespace Util;

// small helper types for testing non-trivial destruction / move semantics
struct CounterObj
{
    static int constructed;
    static int destroyed;
    static int copied;
    static int moved;

    CounterObj() { ++constructed; }
    ~CounterObj() { ++destroyed; }
    CounterObj(const CounterObj&) { ++copied; ++constructed; }
    CounterObj(CounterObj&&) noexcept { ++moved; ++constructed; }
    CounterObj& operator=(const CounterObj&) { ++copied; return *this; }
    CounterObj& operator=(CounterObj&&) noexcept { ++moved; return *this; }
};

int CounterObj::constructed = 0;
int CounterObj::destroyed = 0;
int CounterObj::copied = 0;
int CounterObj::moved = 0;

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
ArrayTest::Run()
{
    // ------------------------------------------------------------------------
    // Basic integer array tests (existing tests preserved)
    // ------------------------------------------------------------------------
    Array<int> array0, array1, array2;
    
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
    for (i = 0; i < 20000; i++)
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

    // ctor from raw pointer (trivially copyable)
    {
        int raw[] = { 10, 11, 12, 13 };
        Array<int> fromPtr(raw, 4);
        VERIFY(fromPtr.Size() == 4);
        VERIFY(fromPtr[0] == 10 && fromPtr[3] == 13);
    }

    // initializer list ctor
    {
        Array<int> il = { 7, 8, 9 };
        VERIFY(il.Size() == 3);
        VERIFY(il[0] == 7 && il[2] == 9);
    }

    // nullptr ctor and IsEmpty
    {
        Array<int> nil(nullptr);
        VERIFY(nil.Size() == 0);
        VERIFY(nil.IsEmpty());
    }

    // variadic Append (Append(first, ...))
    {
        Array<int> v;
        v.Append(1, 2, 3, 4, 5);
        VERIFY(v.Size() == 5);
        for (IndexT j = 0; j < 5; ++j) VERIFY(v[j] == (int)j + 1);
    }

    // Append rvalue and push_back alias
    {
        Array<int> a;
        int x = 42;
        a.Append(x);
        a.Append(std::move(x));
        a.push_back(100);
        VERIFY(a.Size() == 3);
        VERIFY(a[0] == 42);
        VERIFY(a[1] == 42);
        VERIFY(a[2] == 100);
    }

    // Emplace and EmplaceArray
    {
        Array<int> e;
        int &r = e.Emplace();
        r = 9;
        VERIFY(e.Size() == 1);
        VERIFY(e[0] == 9);

        int *p = e.EmplaceArray(3);
        p[0] = 1; p[1] = 2; p[2] = 3;
        VERIFY(e.Size() == 4);
        VERIFY(e[1] == 1 && e[3] == 3);
    }

    // Reserve and Capacity / ByteSize
    {
        Array<int> r;
        r.Reserve(50);
        VERIFY(r.Capacity() >= 50);
        VERIFY(r.ByteSize() == r.Size() * sizeof(int));
    }

    // Find (returns iterator) and FindIndex template
    {
        Array<int> f;
        f.Append(5); f.Append(6); f.Append(7);
        auto it = f.Find(6);
        VERIFY(it && *it == 6);
        VERIFY(f.FindIndex<int>(7) == 2);
        VERIFY(f.FindIndex<int>(123) == InvalidIndex);
    }

    // Insert, EraseSwap, EraseFront, EraseBack, PopFront, PopBack
    {
        Array<int> t;
        t.Append(1); t.Append(2); t.Append(3); t.Append(4);
        t.Insert(2, 99); // 1,2,99,3,4
        VERIFY(t[2] == 99 && t.Size() == 5);

        // EraseSwap on middle element (destroys order)
        t.EraseIndexSwap(1); // swap index 1 with last
        VERIFY(t.Size() == 4);

        // EraseFront
        t.EraseFront();
        VERIFY(t.Size() == 3);

        // PopFront / PopBack
        int pf = t.PopFront();
        int pb = t.PopBack();
        VERIFY(t.Size() == 1);
        (void)pf; (void)pb; // values not important here
    }

    // Erase iterator variants
    {
        Array<int> it;
        it.Append(1); it.Append(2); it.Append(3);
        auto iter = it.Begin();
        ++iter; // points to value 2
        it.Erase(iter);
        VERIFY(it.Size() == 2);
    }

    // EraseRange
    {
        Array<int> er;
        er.Append(1); er.Append(2); er.Append(3); er.Append(4); er.Append(5);
        er.EraseRange(1, 3); // should remove elements at [1,2]
        VERIFY(er.Size() == 3);
        VERIFY(er[0] == 1 && er[1] == 4 && er[2] == 5);
    }

    // SortWithFunc and QuickSortWithFunc
    {
        Array<int> s;
        s.Append(3); s.Append(1); s.Append(2);
        s.SortWithFunc(LessInt);
        VERIFY(s[0] == 1 && s[1] == 2 && s[2] == 3);

        s.Clear();
        s.Append(6); s.Append(4); s.Append(5);
        s.QuickSortWithFunc(QSortIntCmp);
        VERIFY(s[0] == 4 && s[1] == 5 && s[2] == 6);
    }

    // BinarySearchIndex templated KEYTYPE
    {
        Array<int> bs;
        bs.Append(1); bs.Append(2); bs.Append(3); bs.Append(4);
        VERIFY(bs.BinarySearchIndex<int>(3) == 2);
        VERIFY(bs.BinarySearchIndex<int>(99) == InvalidIndex);
    }

    // Resize variants and Extend
    {
        Array<int> re;
        re.Resize(5, 7); // construct new elements with int(7)
        VERIFY(re.Size() == 5);
        for (IndexT j = 0; j < 5; ++j) VERIFY(re[j] == 7);

        re.Resize(2); // shrink, should reduce size
        VERIFY(re.Size() == 2);

        re.Extend(10); // increase capacity/size
        VERIFY(re.Size() == 10);
        re.Resize(0);
        VERIFY(re.Size() == 0);
    }

    // Fit shrinks capacity to size
    {
        Array<int> fit;
        fit.Reserve(100);
        fit.Append(1); fit.Append(2); fit.Append(3);
        fit.Fit();
        VERIFY(fit.Capacity() == fit.Size());
    }

    // Realloc
    {
        Array<int> rc;
        rc.Realloc(8, 4);
        VERIFY(rc.Capacity() == 8);
        VERIFY(rc.Size() == 8);
    }

    // STL-like interface: size(), begin/end(), resize(size_t), clear(), push_back()
    {
        Array<int> stl;
        stl.push_back(11);
        stl.push_back(12);
        VERIFY(stl.size() == 2);
        VERIFY(*stl.begin() == 11);
        stl.resize(5);
        VERIFY(stl.size() == 5);
        stl.clear();
        VERIFY(stl.size() == 0);
    }

    // TypeSize
    {
        Array<int> ts;
        VERIFY(ts.TypeSize() == sizeof(int));
    }

    // IsValidIndex
    {
        Array<int> idx;
        idx.Append(1);
        VERIFY(idx.IsValidIndex(0));
        VERIFY(!idx.IsValidIndex(1));
    }

    // Grow indirectly covered by several operations above

    // ------------------------------------------------------------------------
    // Tests for non-trivially-destructible types (destructors / moves / copies)
    // ------------------------------------------------------------------------
    {
        // reset counters
        CounterObj::constructed = 0;
        CounterObj::destroyed = 0;
        CounterObj::copied = 0;
        CounterObj::moved = 0;

        // use a small-stack array to ensure some elements live in stackElements
        Array<CounterObj, 8> co;
        // Emplace to create default constructed elements
        co.Emplace();
        co.Emplace();
        co.Emplace();
        VERIFY(CounterObj::constructed >= 3);

        // Clear should call destructors for those elements
        co.Clear();
        VERIFY(CounterObj::destroyed >= 3);

        // test move assignment
        CounterObj::constructed = 0; CounterObj::destroyed = 0; CounterObj::copied = 0; CounterObj::moved = 0;
        Array<CounterObj, 8> a1;
        a1.Emplace(); a1.Emplace();
        Array<CounterObj, 8> a2;
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

    // ------------------------------------------------------------------------
    // Adding an element of the array to itself while it has to grow used to
    // read from the freed buffer
    // ------------------------------------------------------------------------
    {
        const Util::String value = "a string long enough to live on the heap";

        Array<Util::String> appendCopy;
        while (appendCopy.Size() < appendCopy.Capacity() || appendCopy.Size() == 0)
            appendCopy.Append(value);
        VERIFY(appendCopy.Size() == appendCopy.Capacity());
        appendCopy.Append(appendCopy[0]);
        VERIFY(appendCopy.Back() == value);

        Array<Util::String> appendMove;
        while (appendMove.Size() < appendMove.Capacity() || appendMove.Size() == 0)
            appendMove.Append(value);
        appendMove.Append(std::move(appendMove[0]));
        VERIFY(appendMove.Back() == value);

        Array<Util::String> appendMany;
        while (appendMany.Size() < appendMany.Capacity() || appendMany.Size() == 0)
            appendMany.Append(value);
        appendMany.Append(appendMany[0], appendMany[1]);
        VERIFY(appendMany[appendMany.Size() - 2] == value);
        VERIFY(appendMany.Back() == value);

        Array<Util::String> appendRange;
        while (appendRange.Size() < appendRange.Capacity() || appendRange.Size() == 0)
            appendRange.Append(value);
        const SizeT oldSize = appendRange.Size();
        appendRange.AppendArray(appendRange.Begin(), 2);
        VERIFY(appendRange.Size() == oldSize + 2);
        VERIFY(appendRange[oldSize] == value && appendRange[oldSize + 1] == value);

        // inserting must use the value before the elements are shifted
        Array<int> insertShift;
        insertShift.Append(1); insertShift.Append(2); insertShift.Append(3);
        insertShift.Insert(0, insertShift[1]);
        VERIFY(insertShift[0] == 2 && insertShift[1] == 1 && insertShift[2] == 2 && insertShift[3] == 3);

        Array<Util::String> insertGrow;
        while (insertGrow.Size() < insertGrow.Capacity() || insertGrow.Size() == 0)
            insertGrow.Append(value);
        insertGrow[1] = "second";
        insertGrow.Insert(0, insertGrow[1]);
        VERIFY(insertGrow[0] == "second");

        Array<Util::String> fill;
        fill.Append(value);
        fill.Fill(0, 64, fill[0]);
        VERIFY(fill.Size() == 64 && fill[63] == value);
    }

    // ------------------------------------------------------------------------
    // Fit() on a small vector must return to the inline storage, otherwise a
    // later move treats the heap buffer as inline storage
    // ------------------------------------------------------------------------
    {
        Array<Util::String, 4> smallarr;
        smallarr.Append("a string long enough to live on the heap");
        smallarr.Fit();
        VERIFY(smallarr.Capacity() == 4);
        Array<Util::String, 4> moved(std::move(smallarr));
        VERIFY(moved.Size() == 1 && moved[0] == "a string long enough to live on the heap");
        smallarr.Append("b");
        VERIFY(smallarr.Size() == 1 && smallarr[0] == "b");

        // spilled to the heap, then shrunk back into the inline storage
        Array<int, 4> spilled;
        for (int i = 0; i < 10; i++) spilled.Append(i);
        spilled.Resize(2);
        spilled.Fit();
        VERIFY(spilled.Capacity() == 4 && spilled[0] == 0 && spilled[1] == 1);
        Array<int, 4> spilledMoved(std::move(spilled));
        VERIFY(spilledMoved.Size() == 2 && spilledMoved[1] == 1);

        // heap only arrays
        Array<int> heapOnly;
        for (int i = 0; i < 40; i++) heapOnly.Append(i);
        heapOnly.Fit();
        VERIFY(heapOnly.Capacity() == 40 && heapOnly[39] == 39);
        heapOnly.Clear();
        heapOnly.Fit();
        VERIFY(heapOnly.Capacity() == 0);
        heapOnly.Append(7);
        VERIFY(heapOnly.Size() == 1 && heapOnly[0] == 7);
    }
}

}; // namespace Test