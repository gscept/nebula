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
    const int capacity = 64;

    Array<String> titles;
    titles.Append("Nausicaä of the Valley of Wind");
    titles.Append("Laputa: The Castle in the Sky");
    titles.Append("My Neighbor Totoro");
    titles.Append("Kiki's Delivery Service");
    titles.Append("Porco Rosso");
    titles.Append("Princess Mononoke");

    // create a hashtable with string keys and IndexT value
    HashTable<String, IndexT> table(capacity);
    this->Verify(table.Size() == 0);
    this->Verify(table.IsEmpty());
    this->Verify(table.Capacity() == capacity);
    this->Verify(!table.Contains("Ein schoener Tag"));

    // populate the hash table
    IndexT i;
    SizeT num = titles.Size();
    for (i = 0; i < num; i++)
    {
        table.Add(titles[i], i);
    }
    this->Verify(!table.IsEmpty());
    this->Verify(table.Size() == titles.Size());
    for (i = 0; i < num; i++)
    {
        this->Verify(table.Contains(titles[i]));
        this->Verify(table[titles[i]] == i);
    }

    // check copy constructor
    HashTable<String, IndexT> copy = table;
    for (i = 0; i < num; i++)
    {
        this->Verify(copy.Contains(titles[i]));
        this->Verify(copy[titles[i]] == i);
    }

    // check erasing
    table.Erase(titles[1]);
    this->Verify(table.Size() == (titles.Size() - 1));
    this->Verify(table.Contains(titles[0]));
    this->Verify(table.Contains(titles[2]));
    this->Verify(table.Contains(titles[3]));
    this->Verify(table.Contains(titles[4]));
    this->Verify(table.Contains(titles[5]));
    this->Verify(!table.Contains(titles[1]));

    // check clearing
    table.Clear();
    this->Verify(table.Size() == 0);
    this->Verify(table.IsEmpty());
}

};