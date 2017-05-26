//------------------------------------------------------------------------------
//  guidtest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "guidtest.h"
#include "util/guid.h"

namespace Test
{
__ImplementClass(Test::GuidTest, 'GUDT', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
GuidTest::Run()
{
    // NOTE: generating GUIDs is only supported under Win32 at the moment
#if __WIN32__
    Guid guid;
    
    // test generation, assignment, conversion from and to string, equality
    this->Verify(!guid.IsValid());
    guid.Generate();
    this->Verify(guid.IsValid());

    Guid guid1 = guid;
    this->Verify(guid1.IsValid());
    this->Verify(guid == guid1);
    
    Guid guid2;
    guid2.Generate();
    this->Verify(guid2 != guid);
    this->Verify(guid2 != guid1);

    String guidStr = guid.AsString();
    String guid1Str = guid1.AsString();
    String guid2Str = guid2.AsString();
    this->Verify(guidStr == guid1Str);
    this->Verify(guidStr != guid2Str);

    guid2 = guid1Str;
    this->Verify(guid1 == guid2);

    // test hash code functionality
    Guid guids[5];
    guids[0].Generate();
    guids[1].Generate();
    guids[2].Generate();
    guids[3].Generate();
    guids[4].Generate();
    HashTable<Guid, String> hashTable;
    hashTable.Add(guids[0], "Guid0");
    hashTable.Add(guids[1], "Guid1");
    hashTable.Add(guids[2], "Guid2");
    hashTable.Add(guids[3], "Guid3");
    hashTable.Add(guids[4], "Guid4");
    this->Verify(hashTable[guids[0]] == "Guid0");
    this->Verify(hashTable[guids[1]] == "Guid1");
    this->Verify(hashTable[guids[2]] == "Guid2");
    this->Verify(hashTable[guids[3]] == "Guid3");
    this->Verify(hashTable[guids[4]] == "Guid4");
#endif
}

}   // namespace Test
