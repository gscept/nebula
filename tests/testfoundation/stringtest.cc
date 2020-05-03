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
    VERIFY(str == "String 1");

    // test += operator
    str += str2;
    VERIFY(str == "String 1String 2");

    // test == and != operator
    str = str1;
    VERIFY(str == str1);
    VERIFY(!(str != str1));

    // test <><=>= operators
    String a = "A";
    String b = "B";
    String c = "A";
    String d = "B";
    VERIFY(a < b);
    VERIFY(a <= b);
    VERIFY(a <= c);
    VERIFY(b > a);
    VERIFY(b >= a);
    VERIFY(b >= d);

    // test [] operators
    str = "ABCDEFG";
    VERIFY(str[0] == 'A');
    VERIFY(str[2] == 'C');
    str[3] = 'X';
    VERIFY(str == "ABCXEFG");

    // test Length, Clear, IsValid, IsEmpty
    str = "ABCDEFGH";
    VERIFY(str.Length() == 8);
    VERIFY(str.IsValid());
    VERIFY(!str.IsEmpty());
    str.Clear();
    VERIFY(str.Length() == 0);
    VERIFY(str.IsEmpty());
    VERIFY(!str.IsValid());

    // test append
    str1 = "ABC";
    str2 = "DEF";
    str1.Append(str2);
    VERIFY(str1 == "ABCDEF");

    // ToLower/ToUpper
    str = "ABCDEFG_+X/ ";
    str.ToLower();
    VERIFY(str == "abcdefg_+x/ ");
    str.ToUpper();
    VERIFY(str == "ABCDEFG_+X/ ");

    // Tokenize
    str = "This is a sentence.";
    Array<String> tokens = str.Tokenize(" .");
    VERIFY(tokens.Size() == 4);
    VERIFY(tokens[0] == "This");
    VERIFY(tokens[1] == "is");
    VERIFY(tokens[2] == "a");
    VERIFY(tokens[3] == "sentence");

    str = "This is a \"spoken sentence\", isn't it?";
    tokens = str.Tokenize(" ,?", '\"');
    VERIFY(tokens.Size() == 6);
    VERIFY(tokens[0] == "This");
    VERIFY(tokens[1] == "is");
    VERIFY(tokens[2] == "a");
    VERIFY(tokens[3] == "spoken sentence");
    VERIFY(tokens[4] == "isn't");
    VERIFY(tokens[5] == "it");

    // ExtractRange
    str = "Rats are perfect pets";
    String range = str.ExtractRange(9, 7);
    VERIFY(range == "perfect");

    // Strip
    str = "Bla bla!?!. Blub";
    str.Strip("?.!");
    VERIFY(str == "Bla bla");

    // FindStringIndex
    str = "Finding a string index in a big string";
    VERIFY(str.FindStringIndex("string", 0) == 10);
    VERIFY(str.FindStringIndex("string", 12) == 32);
    VERIFY(str.FindStringIndex("rotten", 0) == -1);

    // FindCharIndex
    str = "ABCDEFGHIJKJIHGFEDCBA";
    VERIFY(str.FindCharIndex('G', 0) == 6);
    VERIFY(str.FindCharIndex('G', 8) == 14);
    VERIFY(str.FindCharIndex('X', 0) == -1);

    // TerminateAtIndex
    str = "ABCDEFG";
    str.TerminateAtIndex(3);
    VERIFY(str == "ABC");

    // ContainsCharFromSet
    str = "ABCDEFGHIJK";
    VERIFY(str.ContainsCharFromSet("CXD"));
    VERIFY(!str.ContainsCharFromSet("XYZ"));

    // TrimLeft, TrimRight, Trim
    str = " ..*Bla Blub Blob*.. ";
    str.TrimLeft("* .");
    VERIFY(str == "Bla Blub Blob*.. ");
    str.TrimRight(" *.");
    VERIFY(str == "Bla Blub Blob");
    str = " ..*Bla Blub Blob*.. ";
    str.Trim(" .*");
    VERIFY(str == "Bla Blub Blob");

    // SubstituteString
    str = "one two one two";
    str.SubstituteString("one", "two");
    VERIFY(str == "two two two two");

    // SubstituteCharacter
    str = ".dot.dot.dot.";
    str.SubstituteChar('.', ';');
    VERIFY(str == ";dot;dot;dot;");

    // Format
    str.Format("Testing %d %s", 1, "2");
    VERIFY(str == "Testing 1 2");

    // CheckValidCharSet
    str = "123";
    VERIFY(str.CheckValidCharSet("01234567890"));
    str = "123x";
    VERIFY(!str.CheckValidCharSet("01234567890"));

    // ReplaceChars
    str = "12x34y56z";
    str.ReplaceChars("xyz", '.');
    VERIFY(str == "12.34.56.");

    // Concatenate
    Array<String> strArray;
    strArray.Append("one");
    strArray.Append("two");
    strArray.Append("three");
    str = String::Concatenate(strArray, " . ");
    VERIFY(str == "one . two . three");

    // MatchPattern
    VERIFY(String::MatchPattern("gfxlib:dummies/cube.n2", "gfxlib:*.n2"));

    // FIXME: HashCode

    // SetXXX
    str.SetCharPtr("123");
    VERIFY(str == "123");
    str.SetInt(12);
    VERIFY(str == "12");
    str.SetFloat(12.345);
    VERIFY(str == "12.345000");
    str.SetBool(true);
    VERIFY(str == "true");
    str.SetBool(false);
    VERIFY(str == "false");

    // filename stuff
    str = "gfxlib:dummies\\cube.n2";
    VERIFY(str.GetFileExtension() == "n2");
    str.ConvertBackslashes();
    VERIFY(str == "gfxlib:dummies/cube.n2");
    str.StripFileExtension();
    VERIFY(str == "gfxlib:dummies/cube");
    str.Append(".n2");
    VERIFY(str.ExtractFileName() == "cube.n2");
    VERIFY(str.ExtractLastDirName() == "dummies");
    VERIFY(str.ExtractDirName() == "gfxlib:dummies/");
    VERIFY(str.ExtractToLastSlash() == "gfxlib:dummies/");
    str = "gfxlib:?*dummies/()cube";
    str.ReplaceIllegalFilenameChars('_');
    VERIFY(str == "gfxlib___dummies_()cube");
    
    {
        String enc = str.AsBase64();
        String dec = String::FromBase64(enc);
        VERIFY(str == dec);
    }
    
}

}; // namespace Test