//------------------------------------------------------------------------------
//  stringtest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "stringtest.h"
#include "util/string.h"
#include "util/array.h"

namespace Test
{
__ImplementClass(Test::StringTest, 'STRT', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
StringTest::Run()
{
    // create some strings...
    String str1 = "String 1";
    String str2 = "String 2";

    // test operator=
    String str = str1;
    this->Verify(str == "String 1");

    // test += operator
    str += str2;
    this->Verify(str == "String 1String 2");

    // test == and != operator
    str = str1;
    this->Verify(str == str1);
    this->Verify(!(str != str1));

    // test <><=>= operators
    String a = "A";
    String b = "B";
    String c = "A";
    String d = "B";
    this->Verify(a < b);
    this->Verify(a <= b);
    this->Verify(a <= c);
    this->Verify(b > a);
    this->Verify(b >= a);
    this->Verify(b >= d);

    // test [] operators
    str = "ABCDEFG";
    this->Verify(str[0] == 'A');
    this->Verify(str[2] == 'C');
    str[3] = 'X';
    this->Verify(str == "ABCXEFG");

    // test Length, Clear, IsValid, IsEmpty
    str = "ABCDEFGH";
    this->Verify(str.Length() == 8);
    this->Verify(str.IsValid());
    this->Verify(!str.IsEmpty());
    str.Clear();
    this->Verify(str.Length() == 0);
    this->Verify(str.IsEmpty());
    this->Verify(!str.IsValid());

    // test append
    str1 = "ABC";
    str2 = "DEF";
    str1.Append(str2);
    this->Verify(str1 == "ABCDEF");

    // ToLower/ToUpper
    str = "ABCDEFG_+X/ ";
    str.ToLower();
    this->Verify(str == "abcdefg_+x/ ");
    str.ToUpper();
    this->Verify(str == "ABCDEFG_+X/ ");

    // Tokenize
    str = "This is a sentence.";
    Array<String> tokens = str.Tokenize(" .");
    this->Verify(tokens.Size() == 4);
    this->Verify(tokens[0] == "This");
    this->Verify(tokens[1] == "is");
    this->Verify(tokens[2] == "a");
    this->Verify(tokens[3] == "sentence");

    str = "This is a \"spoken sentence\", isn't it?";
    tokens = str.Tokenize(" ,?", '\"');
    this->Verify(tokens.Size() == 6);
    this->Verify(tokens[0] == "This");
    this->Verify(tokens[1] == "is");
    this->Verify(tokens[2] == "a");
    this->Verify(tokens[3] == "spoken sentence");
    this->Verify(tokens[4] == "isn't");
    this->Verify(tokens[5] == "it");

    // ExtractRange
    str = "Rats are perfect pets";
    String range = str.ExtractRange(9, 7);
    this->Verify(range == "perfect");

    // Strip
    str = "Bla bla!?!. Blub";
    str.Strip("?.!");
    this->Verify(str == "Bla bla");

    // FindStringIndex
    str = "Finding a string index in a big string";
    this->Verify(str.FindStringIndex("string", 0) == 10);
    this->Verify(str.FindStringIndex("string", 12) == 32);
    this->Verify(str.FindStringIndex("rotten", 0) == -1);

    // FindCharIndex
    str = "ABCDEFGHIJKJIHGFEDCBA";
    this->Verify(str.FindCharIndex('G', 0) == 6);
    this->Verify(str.FindCharIndex('G', 8) == 14);
    this->Verify(str.FindCharIndex('X', 0) == -1);

    // TerminateAtIndex
    str = "ABCDEFG";
    str.TerminateAtIndex(3);
    this->Verify(str == "ABC");

    // ContainsCharFromSet
    str = "ABCDEFGHIJK";
    this->Verify(str.ContainsCharFromSet("CXD"));
    this->Verify(!str.ContainsCharFromSet("XYZ"));

    // TrimLeft, TrimRight, Trim
    str = " ..*Bla Blub Blob*.. ";
    str.TrimLeft("* .");
    this->Verify(str == "Bla Blub Blob*.. ");
    str.TrimRight(" *.");
    this->Verify(str == "Bla Blub Blob");
    str = " ..*Bla Blub Blob*.. ";
    str.Trim(" .*");
    this->Verify(str == "Bla Blub Blob");

    // SubstituteString
    str = "one two one two";
    str.SubstituteString("one", "two");
    this->Verify(str == "two two two two");

    // SubstituteCharacter
    str = ".dot.dot.dot.";
    str.SubstituteChar('.', ';');
    this->Verify(str == ";dot;dot;dot;");

    // Format
    str.Format("Testing %d %s", 1, "2");
    this->Verify(str == "Testing 1 2");

    // CheckValidCharSet
    str = "123";
    this->Verify(str.CheckValidCharSet("01234567890"));
    str = "123x";
    this->Verify(!str.CheckValidCharSet("01234567890"));

    // ReplaceChars
    str = "12x34y56z";
    str.ReplaceChars("xyz", '.');
    this->Verify(str == "12.34.56.");

    // Concatenate
    Array<String> strArray;
    strArray.Append("one");
    strArray.Append("two");
    strArray.Append("three");
    str = String::Concatenate(strArray, " . ");
    this->Verify(str == "one . two . three");

    // MatchPattern
    this->Verify(String::MatchPattern("gfxlib:dummies/cube.n2", "gfxlib:*.n2"));

    // FIXME: HashCode

    // SetXXX
    str.SetCharPtr("123");
    this->Verify(str == "123");
    str.SetInt(12);
    this->Verify(str == "12");
    str.SetFloat(12.345);
    this->Verify(str == "12.345000");
    str.SetBool(true);
    this->Verify(str == "true");
    str.SetBool(false);
    this->Verify(str == "false");

    // filename stuff
    str = "gfxlib:dummies\\cube.n2";
    this->Verify(str.GetFileExtension() == "n2");
    str.ConvertBackslashes();
    this->Verify(str == "gfxlib:dummies/cube.n2");
    str.StripFileExtension();
    this->Verify(str == "gfxlib:dummies/cube");
    str.Append(".n2");
    this->Verify(str.ExtractFileName() == "cube.n2");
    this->Verify(str.ExtractLastDirName() == "dummies");
    this->Verify(str.ExtractDirName() == "gfxlib:dummies/");
    this->Verify(str.ExtractToLastSlash() == "gfxlib:dummies/");
    str = "gfxlib:?*dummies/()cube";
    str.ReplaceIllegalFilenameChars('_');
    this->Verify(str == "gfxlib___dummies_()cube");
    
    {
        String enc = str.AsBase64();
        String dec = String::FromBase64(enc);
        this->Verify(str == dec);
    }
    
}

}; // namespace Test