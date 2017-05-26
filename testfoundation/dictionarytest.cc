//------------------------------------------------------------------------------
//  dictionarytest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "dictionarytest.h"
#include "util/dictionary.h"

namespace Test
{
__ImplementClass(Test::DictionaryTest, 'DCTT', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
DictionaryTest::Run()
{
    // create a german-to-english dictionary
    Dictionary<String,String> dict;
    this->Verify(dict.IsEmpty());
    this->Verify(dict.Size() == 0);

    // add some values
    dict.Add("Zwei", "Two");
    dict.Add("Eins", "One");
    dict.Add("Drei", "Three");
    dict.Add("Fuenf", "Five");
    dict.Add("Vier", "Four");
    this->Verify(!dict.IsEmpty());
    this->Verify(dict.Size() == 5);
    this->Verify(dict.Contains("Eins"));
    this->Verify(dict.Contains("Drei"));
    this->Verify(dict.Contains("Fuenf"));
    this->Verify(!dict.Contains("Two"));
    this->Verify(!dict.Contains("Sechs"));
    this->Verify(0 == dict.FindIndex("Drei"));
    this->Verify(4 == dict.FindIndex("Zwei"));
    this->Verify(1 == dict.FindIndex("Eins"));
    this->Verify(3 == dict.FindIndex("Vier"));
    this->Verify(2 == dict.FindIndex("Fuenf"));
    this->Verify(dict["Eins"] == "One");
    this->Verify(dict["Zwei"] == "Two");
    this->Verify(dict["Drei"] == "Three");
    this->Verify(dict["Vier"] == "Four");
    this->Verify(dict["Fuenf"] == "Five");
    // NOTE: the following should cause an assertion
    //this->Verify(dict["Sechs"] == "Six");
    this->Verify(dict.KeyAtIndex(0) == "Drei");
    this->Verify(dict.KeyAtIndex(1) == "Eins");
    this->Verify(dict.KeyAtIndex(2) == "Fuenf");
    this->Verify(dict.KeyAtIndex(3) == "Vier");
    this->Verify(dict.KeyAtIndex(4) == "Zwei");
    this->Verify(dict.ValueAtIndex(0) == "Three");
    this->Verify(dict.ValueAtIndex(1) == "One");
    this->Verify(dict.ValueAtIndex(2) == "Five");
    this->Verify(dict.ValueAtIndex(3) == "Four");
    this->Verify(dict.ValueAtIndex(4) == "Two");

    // try changing a value
    dict["Eins"] = "Aaans";
    this->Verify(dict["Eins"] == "Aaans");
    this->Verify(dict.ValueAtIndex(1) == "Aaans");

    // check if Erase() works
    dict.Erase("Eins");
    this->Verify(dict.Size() == 4);
    this->Verify(InvalidIndex == dict.FindIndex("Eins"));
    this->Verify(dict.Contains("Zwei"));
    this->Verify(dict.Contains("Drei"));
    this->Verify(dict.Contains("Vier"));
    this->Verify(dict.Contains("Fuenf"));

    // NOTE: the following should not compile
    // dict.KeyAtIndex(0) = "Zehn";
}

}; // namespace Test

