//------------------------------------------------------------------------------
//  stringtest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "stringtest.h"
#include "util/string.h"
#include "util/array.h"
#if __WIN32__
#include "util/win32/win32stringconverter.h"
#endif

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

    // BeginsWithString (newly fixed)
    str = "Hello World";
    VERIFY(str.BeginsWithString("Hello"));
    VERIFY(!str.BeginsWithString("World"));
    VERIFY(!str.BeginsWithString("Hello World Extra"));
    VERIFY(str.BeginsWithString(""));  // empty prefix always matches
    str = "a";
    VERIFY(str.BeginsWithString("a"));
    VERIFY(!str.BeginsWithString("ab"));

    // EndsWithString (newly fixed)
    str = "Hello World";
    VERIFY(str.EndsWithString("World"));
    VERIFY(!str.EndsWithString("Hello"));
    VERIFY(!str.EndsWithString("Extra Hello World"));
    VERIFY(str.EndsWithString(""));  // empty suffix always matches
    str = "x";
    VERIFY(str.EndsWithString("x"));
    VERIFY(!str.EndsWithString("xy"));

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

    // FindCharIndexReverse
    str = "ABCDEFGHIJKJIHGFEDCBA";
    VERIFY(str.FindCharIndexReverse('A', 0) == 20);
    VERIFY(str.FindCharIndexReverse('G', 0) == 14);
    VERIFY(str.FindCharIndexReverse('X', 0) == -1);
    str = "simple";
    VERIFY(str.FindCharIndexReverse('e', 0) == 5);
    VERIFY(str.FindCharIndexReverse('s', 0) == 0);

    // ExtractToEnd
    str = "The quick brown fox";
    VERIFY(str.ExtractToEnd(4) == "quick brown fox");
    VERIFY(str.ExtractToEnd(0) == str);
    str = "X";
    VERIFY(str.ExtractToEnd(1) == "");

    // Move constructor
    String moveStr1 = "Move me!";
    String moveStr2 = std::move(moveStr1);
    VERIFY(moveStr2 == "Move me!");
    VERIFY(moveStr1.IsEmpty());  // moved-from should be empty

    // Move assignment
    String moveStr3 = "Original";
    String moveStr4 = "To be replaced";
    moveStr4 = std::move(moveStr3);
    VERIFY(moveStr4 == "Original");
    VERIFY(moveStr3.IsEmpty());  // moved-from should be empty

    // Copy constructor
    String copyStr1 = "Copy me";
    String copyStr2 = copyStr1;
    VERIFY(copyStr2 == "Copy me");
    VERIFY(copyStr1 == "Copy me");  // original unchanged

    // AppendPath
    str = "root/dir1";
    str.AppendPath("file.txt");
    VERIFY(str == "root/dir1/file.txt");
    str = "";
    str.AppendPath("file.txt");
    VERIFY(str == "file.txt");
    str = "path/";
    str.AppendPath("file.txt");
    VERIFY(str == "path/file.txt");
    // Static AppendPath
    str = String::AppendPath("root", "sub/file");
    VERIFY(str == "root/sub/file");

    // Capitalize
    str = "hello";
    str.Capitalize();
    VERIFY(str == "Hello");
    str = "aLREADY cAPITAL";
    str.Capitalize();
    VERIFY(str == "ALREADY cAPITAL");

    // CamelCaseToWords
    str = "myVariableName";
    str.CamelCaseToWords();
    VERIFY(str == "my Variable Name");

    // StripAssignPrefix
    str = "tex:myfile.txt";
    str.StripAssignPrefix();
    VERIFY(str == "myfile.txt");
    str = "noprefix";
    str.StripAssignPrefix();
    VERIFY(str == "noprefix");

    // ChangeAssignPrefix
    str = "tex:file.txt";
    str.ChangeAssignPrefix("gfx");
    VERIFY(str == "gfx:file.txt");

    // Integer conversions
    str.SetInt(42);
    VERIFY(str.AsInt() == 42);
    str.SetInt(-100);
    VERIFY(str.AsInt() == -100);

    // Float conversion
    str.SetFloat(3.14f);
    float f = str.AsFloat();
    VERIFY(f > 3.0f && f < 3.2f);

    // IsValidInt / IsValidFloat / IsValidBool
    str = "12345";
    VERIFY(str.IsValidInt());
    str = "3.14";
    VERIFY(str.IsValidFloat());
    str = "true";
    VERIFY(str.IsValidBool());
    str = "not a number";
    VERIFY(!str.IsValidInt());
    VERIFY(!str.IsValidFloat());
    VERIFY(!str.IsValidBool());

    // CopyToBuffer edge cases
    char buf[10];
    str = "12345";
    VERIFY(str.CopyToBuffer(buf, sizeof(buf)));
    VERIFY(strcmp(buf, "12345") == 0);
    str = "1234567890123";
    VERIFY(!str.CopyToBuffer(buf, sizeof(buf)));  // too small

    // FromBlob/AsBlob round-trip
    {
        Util::Blob blob;
        blob.Reserve(5);
        unsigned char* data = (unsigned char*)blob.GetPtr();
        data[0] = 0xAB;
        data[1] = 0xCD;
        data[2] = 0xEF;
        data[3] = 0x12;
        data[4] = 0x34;
        String hexStr = String::FromBlob(blob);
        Util::Blob blob2 = hexStr.AsBlob();
        VERIFY(blob2.Size() == blob.Size());
        VERIFY(blob2 == blob);
    }

    // CheckFileExtension
    str = "file.txt";
    VERIFY(str.CheckFileExtension("txt"));
    VERIFY(!str.CheckFileExtension("doc"));

    // StripSubpath
    str = "root/sub/file.txt";
    str = str.StripSubpath("root/sub");
    VERIFY(str == "file.txt");

    // Empty string edge cases
    str = "";
    VERIFY(str.IsEmpty());
    VERIFY(str.Length() == 0);
    VERIFY(!str.IsValid());
    str.Append(" ");
    VERIFY(!str.IsEmpty());
    str.ToUpper();
    VERIFY(str == " ");
    str.Clear();
    VERIFY(str.IsEmpty());

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
    

#if __WIN32__
    LPCWSTR wideString = L"foo/";
    Util::String utf8str = Win32::Win32StringConverter::WideToUTF8((const ushort*)wideString);
    VERIFY(utf8str.Length() == 4);
    VERIFY(utf8str[0] == 'f');
    VERIFY(utf8str[1] == 'o');
    VERIFY(utf8str[2] == 'o');
    VERIFY(utf8str[3] == '/');
    VERIFY(utf8str.AsCharPtr()[4] == '\0');

    utf8str.TrimRight("/");
    VERIFY(utf8str == "foo");
    VERIFY(utf8str.Length() == 3);
    VERIFY(utf8str.AsCharPtr()[3] == '\0');
#endif
}

}; // namespace Test