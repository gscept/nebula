//------------------------------------------------------------------------------
//  hashtabletest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "hashtabletest.h"
#include "util/hashtable.h"

namespace Test
{
__ImplementClass(Test::HashTableTest, 'HSTT', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
HashTableTest::Run()
{
    constexpr int capacity = 64;

    Array<String> titles;
    titles.Append("Nausicaä of the Valley of Wind");
    titles.Append("Laputa: The Castle in the Sky");
    titles.Append("My Neighbor Totoro");
    titles.Append("Kiki's Delivery Service");
    titles.Append("Porco Rosso");
    titles.Append("Princess Mononoke");

    // create a hashtable with string keys and IndexT value
    HashTable<String, IndexT, capacity> table;
    VERIFY(table.Size() == 0);
    VERIFY(table.IsEmpty());
    VERIFY(table.Capacity() == capacity);
    VERIFY(!table.Contains("Ein schoener Tag"));

    // populate the hash table
    IndexT i;
    SizeT num = titles.Size();
    for (i = 0; i < num; i++)
    {
        table.Add(titles[i], i);
    }
    VERIFY(!table.IsEmpty());
    VERIFY(table.Size() == titles.Size());
    for (i = 0; i < num; i++)
    {
        VERIFY(table.Contains(titles[i]));
        VERIFY(table[titles[i]] == i);
    }

    // check copy constructor
    HashTable<String, IndexT, capacity> copy = table;
    for (i = 0; i < num; i++)
    {
        VERIFY(copy.Contains(titles[i]));
        VERIFY(copy[titles[i]] == i);
    }

    // check erasing
    table.Erase(titles[1]);
    VERIFY(table.Size() == (titles.Size() - 1));
    VERIFY(table.Contains(titles[0]));
    VERIFY(table.Contains(titles[2]));
    VERIFY(table.Contains(titles[3]));
    VERIFY(table.Contains(titles[4]));
    VERIFY(table.Contains(titles[5]));
    VERIFY(!table.Contains(titles[1]));

    // check clearing
    table.Clear();
    VERIFY(table.Size() == 0);
    VERIFY(table.IsEmpty());
}

};