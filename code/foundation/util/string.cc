//------------------------------------------------------------------------------
//  string.cc
//  (C) 2006 RadonLabs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "util/string.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "base64/base64.h"
#include "blob.h"
#include "math/mat4.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/quat.h"
#include "math/transform44.h"

//------------------------------------------------------------------------------
/**
    Literal constructor form string, to use "foobar"_str will automatically construct a String
*/
Util::String
operator ""_str(const char* c, std::size_t s)
{
    return Util::String(c);
}

namespace Util
{

//------------------------------------------------------------------------------
/**
*/
bool
String::CopyToBuffer(char* buf, SizeT bufSize) const
{
    n_assert(0 != buf);
    n_assert(bufSize > 1);

    #if __WIN32__
        HRESULT hr = StringCchCopy(buf, bufSize, this->AsCharPtr());
        return SUCCEEDED(hr);    
    #else
        strncpy(buf, this->AsCharPtr(), bufSize - 1);
        buf[bufSize - 1] = 0;
        return this->strLen < bufSize;
    #endif
}

//------------------------------------------------------------------------------
/**
*/
void __cdecl
String::Format(const char* fmtString, ...)
{
    va_list argList;
    va_start(argList, fmtString);
    char buf[4096]; // an 4 kByte buffer
    #if __WIN32__
        // need to use non-CRT thread safe function under Win32
        StringCchVPrintf(buf, sizeof(buf), fmtString, argList);
    #elif (__OSX__ || __linux__)
        vsnprintf(buf, sizeof(buf), fmtString, argList);
    #else
        _vsnprintf(buf, sizeof(buf), fmtString, argList);
    #endif
    *this = buf;
    va_end(argList);
}

//------------------------------------------------------------------------------
/**
*/
void __cdecl
String::FormatArgList(const char* fmtString, va_list argList)
{
    char buf[4096]; // an 4 kByte buffer
    #if __WIN32__
        // need to use non-CRT thread safe function under Win32
        StringCchVPrintf(buf, sizeof(buf), fmtString, argList);
    #elif (__OSX__ || __linux__)
        vsnprintf(buf, sizeof(buf), fmtString, argList);
    #else
        _vsnprintf(buf, sizeof(buf), fmtString, argList);
    #endif
    *this = buf;
}

//------------------------------------------------------------------------------
/**
*/
String
String::Sprintf(const char* fmtString, ...)
{
    String format;
    va_list argList;
    va_start(argList, fmtString);
    format.FormatArgList(fmtString, argList);
    va_end(argList);
    return format;
}

//------------------------------------------------------------------------------
/**
    Sets a new string content. This will handle all special cases and try
    to minimize heap allocations as much as possible.
*/
void
String::Set(const char* str, SizeT length)
{
    if (0 == str)
    {
        // a null string pointer is valid as a special case
        this->Delete();
        this->localBuffer[0] = 0;
    }
    else if ((0 == this->heapBuffer) && (length < LocalStringSize))
    {
        // the new contents fits into the local buffer,
        // however, if there is already an external buffer
        // allocated, we use the external buffer!
        this->Delete();
        Memory::Copy(str, this->localBuffer, length);
        this->localBuffer[length] = 0;
    }
    else if (length < this->heapBufferSize)
    {
        // the new contents fits into the existing buffer
        Memory::Copy(str, this->heapBuffer, length);
        this->heapBuffer[length] = 0;
        this->localBuffer[0] = 0;
    }
    else
    {
        // need to allocate bigger heap buffer
        this->Alloc(length + 1);
        Memory::Copy(str, this->heapBuffer, length);
        this->heapBuffer[length] = 0;
        this->localBuffer[0] = 0;
    }
    this->strLen = length;
}

//------------------------------------------------------------------------------
/**
*/
void
String::AppendRange(const char* append, SizeT length)
{
    n_assert(append);
    if (length > 0)
    {
        SizeT newLength = this->strLen + length;
        
        // if there is an external buffer allocated, we need to stay there
        // even if the resulting string is smaller then LOCALSTRINGSIZE,
        // a small string in an external buffer may be the result
        // of a call to Reserve()
        if ((0 == this->heapBuffer) && (newLength < LocalStringSize))
        {
            // the result fits into the local buffer
            Memory::Copy(append, this->localBuffer + this->strLen, length);
            this->localBuffer[newLength] = 0;
        }
        else if (newLength < this->heapBufferSize)
        {
            // the result fits into the existing buffer
            Memory::Copy(append, this->heapBuffer + this->strLen, length);
            this->heapBuffer[newLength] = 0;
        }
        else
        {
            // need to re-allocate
            this->Realloc(newLength + newLength / 2);
            Memory::Copy(append, this->heapBuffer + this->strLen, length);
            this->heapBuffer[newLength] = 0;
        }
        this->strLen = newLength;
    }
}

//------------------------------------------------------------------------------
/**
    Tokenize the string into a String array. 

    @param  whiteSpace      a string containing the whitespace characters
    @return                 a string array of tokens
*/
SizeT
String::Tokenize(const String& whiteSpace, Array<String>& outTokens) const
{
    outTokens.Clear();
    String str(*this);
    char* ptr = const_cast<char*>(str.AsCharPtr());
    const char* token;
    while (0 != (token = strtok(ptr, whiteSpace.AsCharPtr())))
    {
        outTokens.Append(token);
        ptr = 0;
    }
    return outTokens.Size();
}

//------------------------------------------------------------------------------
/**
    This is the slow-but-convenient Tokenize() method. Slow since the
    returned string array will be constructed anew with every method call.
    Consider the Tokenize() method which takes a string array as input,
    since this may allow reusing of the array, reducing heap allocations.
*/
Array<String>
String::Tokenize(const String& whiteSpace) const
{
    Array<String> tokens;
    this->Tokenize(whiteSpace, tokens);
    return tokens;
}

//------------------------------------------------------------------------------
/**
    Tokenize a string, but keeps the string within the fence-character
    intact. For instance for the sentence:

    He said: "I don't know."

    A Tokenize(" ", '"', tokens) would return:

    token 0:    He
    token 1:    said:
    token 2:    I don't know.
*/
SizeT
String::Tokenize(const String& whiteSpace, char fence, Array<String>& outTokens) const
{
    outTokens.Clear();
    String str(*this);
    char* ptr = const_cast<char*>(str.AsCharPtr());    
    char* end = ptr + strlen(ptr);
    while (ptr < end)
    {
        char* c;

        // skip white space
        while (*ptr && strchr(whiteSpace.AsCharPtr(), *ptr))
        {
            ptr++;
        }
        if (*ptr)
        {
            // check for fenced area
            if ((fence == *ptr) && (0 != (c = strchr(++ptr, fence))))
            {
                *c++ = 0;
                outTokens.Append(ptr);
                ptr = c;
            }
            else if (0 != (c = strpbrk(ptr, whiteSpace.AsCharPtr())))
            {
                *c++ = 0;
                outTokens.Append(ptr);
                ptr = c;
            }
            else
            {
                outTokens.Append(ptr);
                break;
            }
        }
    }
    return outTokens.Size();
}

//------------------------------------------------------------------------------
/**
    Slow version of Tokenize() with fence character. See above Tokenize()
    for details.
*/
Array<String>
String::Tokenize(const String& whiteSpace, char fence) const
{
    Array<String> tokens;
    this->Tokenize(whiteSpace, fence, tokens);
    return tokens;
}

//------------------------------------------------------------------------------
/**
    Extract a substring range.
*/
String
String::ExtractRange(IndexT from, SizeT numChars) const
{
    n_assert(from <= this->strLen);
    n_assert((from + numChars) <= this->strLen);
    const char* str = this->AsCharPtr();
    String newString;
    newString.Set(&(str[from]), numChars);
    return newString;
}

//------------------------------------------------------------------------------
/**
    Extract a substring until the end of the original string.
*/
String
String::ExtractToEnd(IndexT fromIndex) const
{
    return this->ExtractRange(fromIndex, this->strLen - fromIndex);
}

//------------------------------------------------------------------------------
/**
    Terminates the string at the first occurance of one of the characters
    in charSet.
*/
void
String::Strip(const String& charSet)
{
    char* str = const_cast<char*>(this->AsCharPtr());
    char* ptr = strpbrk(str, charSet.AsCharPtr());
    if (ptr)
    {
        *ptr = 0;
    }
    this->strLen = (SizeT) strlen(this->AsCharPtr());
}

//------------------------------------------------------------------------------
/**
    Return the index of a substring, or InvalidIndex if not found.
*/
IndexT
String::FindStringIndex(const String& s, IndexT startIndex) const
{
    n_assert(startIndex < this->strLen);
    n_assert(s.IsValid());
    IndexT i;
    for (i = startIndex; i < this->strLen; i++)
    {
        if ((this->strLen - i) < s.strLen)
        {
            break;
        }

        if (strncmp(&(this->AsCharPtr()[i]), s.AsCharPtr(), s.strLen) == 0)
        {
            return i;
        }
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    Return index of character in string, or InvalidIndex if not found.
*/
IndexT
String::FindCharIndex(char c, IndexT startIndex) const
{
    if (this->strLen > 0)
    {
        n_assert(startIndex < this->strLen);
        const char* ptr = strchr(this->AsCharPtr() + startIndex, c);
        if (ptr)
        {
            return IndexT(ptr - this->AsCharPtr());
        }
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
bool
String::BeginsWithString(const String& s) const
{
    return FindStringIndex(s) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
bool
String::EndsWithString(const String& s) const
{
    return FindStringIndex(s, s.Length()) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    Removes all characters in charSet from the left side of the string.
*/
void
String::TrimLeft(const String& charSet)
{
    n_assert(charSet.IsValid());
    if (this->IsValid())
    {
        SizeT charSetLen = charSet.strLen;
        IndexT thisIndex = 0;
        bool stopped = false;
        while (!stopped && (thisIndex < this->strLen))
        {
            IndexT charSetIndex;
            bool match = false;
            for (charSetIndex = 0; charSetIndex < charSetLen; charSetIndex++)
            {
                if ((*this)[thisIndex] == charSet[charSetIndex])
                {
                    // a match
                    match = true;
                    break;
                }
            }
            if (!match)
            {
                // stop if no match
                stopped = true;
            }
            else
            {
                // a match, advance to next character
                ++thisIndex;
            }
        }
        String trimmedString(&(this->AsCharPtr()[thisIndex]));
        *this = trimmedString;
    }
}

//------------------------------------------------------------------------------
/**
    Removes all characters in charSet from the right side of the string.
*/
void
String::TrimRight(const String& charSet)
{
    n_assert(charSet.IsValid());
    if (this->IsValid())
    {
        SizeT charSetLen = charSet.strLen;
        int thisIndex = this->strLen - 1;   // NOTE: may not be unsigned (thus not IndexT!)
        bool stopped = false;
        while (!stopped && (thisIndex >= 0))
        {
            IndexT charSetIndex;
            bool match = false;
            for (charSetIndex = 0; charSetIndex < charSetLen; charSetIndex++)
            {
                if ((*this)[thisIndex] == charSet[charSetIndex])
                {
                    // a match
                    match = true;
                    break;
                }
            }
            if (!match)
            {
                // stop if no match
                stopped = true;
            }
            else
            {
                // a match, advance to next character
                --thisIndex;
            }
        }
        String trimmedString;
        trimmedString.Set(this->AsCharPtr(), thisIndex + 1);
        *this = trimmedString;
    }
}

//------------------------------------------------------------------------------
/**
    Trim both sides of a string.
*/
void
String::Trim(const String& charSet)
{
    this->TrimLeft(charSet);
    this->TrimRight(charSet);
}

//------------------------------------------------------------------------------
/**
    Substitute every occurance of origStr with substStr.
*/
void
String::SubstituteString(const String& matchStr, const String& substStr)
{
    n_assert(matchStr.IsValid());

    const char* ptr = this->AsCharPtr();
    SizeT matchStrLen = matchStr.strLen;
    String dest;

    // walk original string for occurances of str
    const char* occur;
    while (0 != (occur = strstr(ptr, matchStr.AsCharPtr())))
    {
        // append string fragment until match
        dest.AppendRange(ptr, SizeT(occur - ptr));

        // append replacement string
        if (substStr.Length() > 0)
        {
            dest.Append(substStr);
        }

        // adjust source pointer
        ptr = occur + matchStrLen;
    }
    dest.Append(ptr);
    *this = dest;
}

//------------------------------------------------------------------------------
/**
    Return a String object containing the last directory of the path, i.e.
    a category.

    - 17-Feb-04     floh    fixed a bug when the path ended with a slash
*/
String
String::ExtractLastDirName() const
{
    String pathString(*this);
    char* lastSlash = pathString.GetLastSlash();
    
    // special case if path ends with a slash
    if (lastSlash)
    {
        if (0 == lastSlash[1])
        {
            *lastSlash = 0;
            lastSlash = pathString.GetLastSlash();
        }

        char* secLastSlash = 0;
        if (0 != lastSlash)
        {
            *lastSlash = 0; // cut filename
            secLastSlash = pathString.GetLastSlash();
            if (secLastSlash)
            {
                *secLastSlash = 0;
                return String(secLastSlash+1);
            }
        }
    }
    return "";
}

//------------------------------------------------------------------------------
/**
    Return a String object containing the part before the last
    directory separator.
    
    NOTE: I left my fix in that returns the last slash (or colon), this was 
    necessary to tell if a dirname is a normal directory or an assign.     

    - 17-Feb-04     floh    fixed a bug when the path ended with a slash
*/
String
String::ExtractDirName() const
{
    String pathString(*this);
    char* lastSlash = pathString.GetLastSlash();

    // special case if path ends with a slash
    if (lastSlash)
    {
        if (0 == lastSlash[1])
        {
            *lastSlash = 0;
            lastSlash = pathString.GetLastSlash();
        }
        if (lastSlash)
        {
            *++lastSlash = 0;
        }
    }
    pathString.strLen = (SizeT) strlen(pathString.AsCharPtr());
    return pathString;
}

//------------------------------------------------------------------------------
/**
    Pattern-matching, TCL-style.
*/
bool
String::MatchPattern(const String& string, const String& pattern)
{
    const char* str = string.AsCharPtr();
    const char* pat = pattern.AsCharPtr();

    char c2;
    bool always = true;
    while (always != false)
    {
        if (*pat == 0) 
        {
            if (*str == 0) return true;
            else           return false;
        }
        if ((*str == 0) && (*pat != '*')) return false;
        if (*pat=='*') 
        {
            pat++;
            if (*pat==0) return true;
            while (always)
            {
                if (String::MatchPattern(str, pat)) return true;
                if (*str==0) return false;
                str++;
            }
        }
        if (*pat=='?') goto match;
        if (*pat=='[') 
        {
            pat++;
            while (always) 
            {
                if ((*pat==']') || (*pat==0)) return false;
                if (*pat==*str) break;
                if (pat[1] == '-') 
                {
                    c2 = pat[2];
                    if (c2==0) return false;
                    if ((*pat<=*str) && (c2>=*str)) break;
                    if ((*pat>=*str) && (c2<=*str)) break;
                    pat+=2;
                }
                pat++;
            }
            while (*pat!=']') 
            {
                if (*pat==0) 
                {
                    pat--;
                    break;
                }
                pat++;
            }
            goto match;
        }
    
        if (*pat=='\\') 
        {
            pat++;
            if (*pat==0) return false;
        }
        if (*pat!=*str) return false;

match:
        pat++;
        str++;
    }
    // can't happen
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
String::ReplaceChars(const String& charSet, char replacement)
{
    n_assert(charSet.IsValid());
    char* ptr = const_cast<char*>(this->AsCharPtr());
    char c;
    while (0 != (c = *ptr))
    {
        if (strchr(charSet.AsCharPtr(), c))
        {
            *ptr = replacement;
        }
        ptr++;
    }
}

#if !__OSX__    
//------------------------------------------------------------------------------
/**
    Returns content as mat4. Note: this method doesn't check whether the
    contents is actually a valid mat4. Use the IsValidMat4() method
    for this!
*/
Math::mat4
String::AsMat4() const
{
    Array<String> tokens(16, 0); 
    this->Tokenize(", \t\n", tokens);
    n_assert(tokens.Size() == 16);
    Math::mat4 m(Math::vec4(tokens[0].AsFloat(),  tokens[1].AsFloat(),  tokens[2].AsFloat(),  tokens[3].AsFloat()),
                 Math::vec4(tokens[4].AsFloat(),  tokens[5].AsFloat(),  tokens[6].AsFloat(),  tokens[7].AsFloat()),
                 Math::vec4(tokens[8].AsFloat(),  tokens[9].AsFloat(),  tokens[10].AsFloat(), tokens[11].AsFloat()),
                 Math::vec4(tokens[12].AsFloat(), tokens[13].AsFloat(), tokens[14].AsFloat(), tokens[15].AsFloat()));
    return m;
}

//------------------------------------------------------------------------------
/**

*/
Math::transform44
String::AsTransform44() const
{
    Array<String> tokens(6, 0);
    this->Tokenize("|", tokens);
    n_assert(tokens.Size() == 6);
    Math::transform44 t;
    t.setposition(tokens[0].AsVec3());
    t.setrotate(tokens[1].AsVec4());
    t.setscale(Math::vec3(tokens[2].AsFloat()));
    t.setrotatepivot(tokens[3].AsVec3());
    t.setscalepivot(tokens[4].AsVec3());
    t.setoffset(tokens[5].AsMat4());
    return t;
}
#endif
    
//------------------------------------------------------------------------------
/**
*/
String
String::Concatenate(const Array<String>& strArray, const String& whiteSpace)
{
    String res;
    res.Reserve(256);
    IndexT i;
    SizeT num = strArray.Size();
    for (i = 0; i < num; i++)
    {
        res.Append(strArray[i]);
        if ((i + 1) < num)
        {
            res.Append(whiteSpace);
        }
    }
    return res;
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetCharPtr(const char* s)
{
    SizeT len = 0;
    if (s)
    {
        len = (SizeT) strlen((const char*)s);
    }
    this->Set(s, len);
}

//------------------------------------------------------------------------------
/**
*/
void
String::Append(const char* str)
{
    n_assert(0 != str);
    this->AppendRange(str, (SizeT) strlen(str));
}

//------------------------------------------------------------------------------
/**
    Terminates the string at the given index.
*/
void
String::TerminateAtIndex(IndexT index)
{
    n_assert(index < this->strLen);
    char* ptr = const_cast<char*>(this->AsCharPtr());
    ptr[index] = 0;
    this->strLen = (SizeT) strlen(ptr);
}

//------------------------------------------------------------------------------
/**
    Remove the file extension.
*/
void
String::StripFileExtension()
{
    char* str = const_cast<char*>(this->AsCharPtr());
    char* ext = strrchr(str, '.');
    if (ext)
    {
        *ext = 0;
        this->strLen = (SizeT) strlen(this->AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
*/
void
String::StripAssignPrefix()
{
    char* str = const_cast<char*>(this->AsCharPtr());
    char* ext = strchr(str, ':');
    if (ext)
    {
        this->SetCharPtr(ext + 1);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool 
operator==(const String& a, const String& b)
{
    return strcmp(a.AsCharPtr(), b.AsCharPtr()) == 0;
}

//------------------------------------------------------------------------------
/**
*/
bool 
operator==(const String& a, const char* cStr)
{
    n_assert(0 != cStr);
    return strcmp(a.AsCharPtr(), cStr) == 0;
}

//------------------------------------------------------------------------------
/**
*/
bool 
operator==(const char* cStr, const String& b)
{
    n_assert(0 != cStr);
    return strcmp(cStr, b.AsCharPtr()) == 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
operator==(const String& a, std::nullptr_t)
{
    return a.Length() == 0;
}

//------------------------------------------------------------------------------
/**
*/
bool 
operator != (const String& a, const String& b)
{
    return strcmp(a.AsCharPtr(), b.AsCharPtr()) != 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
operator < (const String& a, const String& b)
{
    return strcmp(a.AsCharPtr(), b.AsCharPtr()) < 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
operator > (const String& a, const String& b)
{
    return strcmp(a.AsCharPtr(), b.AsCharPtr()) > 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
operator <= (const String& a, const String& b)
{
    return strcmp(a.AsCharPtr(), b.AsCharPtr()) <= 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
operator >= (const String& a, const String& b)
{
    return strcmp(a.AsCharPtr(), b.AsCharPtr()) >= 0;
}

//------------------------------------------------------------------------------
/**
*/
void
String::ToLower()
{
    char* str = const_cast<char*>(this->AsCharPtr());
    char c;
    while (0 != (c = *str))
    {
        *str++ = (char) tolower(c);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
String::ToUpper()
{
    char* str = const_cast<char*>(this->AsCharPtr());
    char c;
    while (0 != (c = *str))
    {
        *str++ = (char) toupper(c);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
String::Capitalize()
{
    char* str = const_cast<char*>(this->AsCharPtr());    
    *str = (char) toupper(*str);
}

//------------------------------------------------------------------------------
/**
*/
void
String::CamelCaseToWords()
{
    const char* str = this->AsCharPtr();
    Util::String newString;
    newString.AppendChar(str[0]);
    // always skip first letter
    for (IndexT i = 1; i < this->strLen; i++)
    {
        if (IsUpper(str[i]) && !IsUpper(str[i - 1]))
        {
            newString.AppendChar(' ');
        }
        newString.AppendChar(str[i]);
    }
    *this = newString;
}

//------------------------------------------------------------------------------
/**
    Returns true if string contains one of the characters from charset.
*/
bool
String::ContainsCharFromSet(const String& charSet) const
{
    char* str = const_cast<char*>(this->AsCharPtr());
    char* ptr = strpbrk(str, charSet.AsCharPtr());
    return (0 != ptr);
}

//------------------------------------------------------------------------------
/**
    @return     string representing the filename extension (maybe empty)
*/
String
String::GetFileExtension() const
{
    const char* str = this->AsCharPtr();
    const char* ext = strrchr(str, '.');
    if (ext)
    {
        ext++;
        return String(ext);
    }
    return String("");
}

//------------------------------------------------------------------------------
/**
    Get a pointer to the last directory separator.
*/
char*
String::GetLastSlash() const
{
    const char* s = this->AsCharPtr();
    const char* lastSlash = strrchr(s, '/');
    if (0 == lastSlash) lastSlash = strrchr(s, '\\');
    if (0 == lastSlash) lastSlash = strrchr(s, ':');
    return const_cast<char*>(lastSlash);
}

//------------------------------------------------------------------------------
/**
*/
bool
String::IsValidBool() const
{
    static const char* bools[] = {
        "no", "yes", "off", "on", "false", "true", 0
    };
    IndexT i = 0;
    while (bools[i] != 0)
    {
        if (0 == n_stricmp(bools[i], this->AsCharPtr()))
        {
            return true;
        }
        i++;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Returns content as integer. Note: this method doesn't check whether the
    contents is actually a valid integer. Use the IsValidInteger() method
    for this!
*/
int
String::AsInt() const
{
    return atoi(this->AsCharPtr());
}

//------------------------------------------------------------------------------
/**
Returns content as integer. Note: this method doesn't check whether the
contents is actually a valid integer. Use the IsValidInteger() method
for this!
*/
long long
String::AsLongLong() const
{
    return atoll(this->AsCharPtr());
}

//------------------------------------------------------------------------------
/**
    Returns content as float. Note: this method doesn't check whether the
    contents is actually a valid float. Use the IsValidInt() method
    for this!
*/
float
String::AsFloat() const
{
    return float(atof(this->AsCharPtr()));
}

//------------------------------------------------------------------------------
/**
*/
bool
String::AsBool() const
{
    static const char* bools[] = {
        "no", "yes", "off", "on", "false", "true", "0","1",0
    };
    IndexT i = 0;
    while (bools[i] != 0)
    {
        if (0 == n_stricmp(bools[i], this->AsCharPtr()))
        {
            return 1 == (i & 1);
        }
        i++;
    }
    n_error("Invalid string value for bool!");
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
String::IsDigit(char c)
{
    return (0 != isdigit(int(c)));
}

//------------------------------------------------------------------------------
/**
*/
bool
String::IsAlpha(char c)
{
    return (0 != isalpha(int(c)));
}

//------------------------------------------------------------------------------
/**
*/
bool
String::IsAlNum(char c)
{
    return (0 != isalnum(int(c)));
}

//------------------------------------------------------------------------------
/**
*/
bool
String::IsLower(char c)
{
    return (0 != islower(int(c)));
}

//------------------------------------------------------------------------------
/**
*/
bool
String::IsUpper(char c)
{
    return (0 != isupper(int(c)));
}

//------------------------------------------------------------------------------
/**
*/
int
String::StrCmp(const char* str0, const char* str1)
{
    n_assert(str0 && str1);
    return strcmp(str0, str1);
}

//------------------------------------------------------------------------------
/**
*/
int
String::StrLen(const char* str)
{
    n_assert(str);
    return (int)strlen(str);
}

//------------------------------------------------------------------------------
/**
*/
const char*
String::StrChr(const char* str, int c)
{
    n_assert(str);
    return strchr(str, c);
}

//------------------------------------------------------------------------------
/**
*/
Dictionary<String,String>
String::ParseKeyValuePairs(const String& str)
{
    Dictionary<String,String> res;
    Array<String> tokens = str.Tokenize(" \t\n=", '"');
    n_assert(0 == (tokens.Size() & 1)); // num tokens must be even
    IndexT i;
    for (i = 0; i < tokens.Size(); i += 2)
    {
        res.Add(tokens[i], tokens[i + 1]);
    }
    return res;
}

//------------------------------------------------------------------------------
/**
*/
void 
String::ChangeFileExtension(const Util::String& newExt)
{
    this->StripFileExtension();
    this->Append("." + newExt);
}

//------------------------------------------------------------------------------
/**
*/
void
String::ChangeAssignPrefix(const Util::String& newPref)
{
    this->StripAssignPrefix();
    *this = newPref + ":" + *this;
}

//------------------------------------------------------------------------------
/**
*/
String
String::FromBlob(const Util::Blob& b)
{
    n_assert(b.IsValid());
    
    unsigned char * buf = (unsigned char*)Memory::Alloc(Memory::ScratchHeap,b.Size() * 2 +1);
    buf[b.Size() * 2] = 0;
    static char hex[]= "0123456789abcdef";        
    
    const unsigned char * ptr = (const unsigned char*)b.GetPtr();
    for(size_t i = 0 ; i < b.Size(); i ++)
    {
        buf[i * 2 + 0] = hex[ptr[i] >> 4  ];
        buf[i * 2 + 1] = hex[ptr[i] & 0x0F];
    }

    Util::String str;
    str.SetCharPtr((const char*)buf);
    return str;
}


//------------------------------------------------------------------------------
/**
*/
Util::Blob 
String::AsBlob() const
{
    Util::Blob b;
    SizeT s = this->heapBufferSize >> 1;
    b.Reserve(s);
    unsigned char * ptr = (unsigned char*)b.GetPtr();
    SizeT c = 0;
    for(SizeT i = 0 ; i < s; i++)
    {
        unsigned char temp = this->heapBuffer[c++];

        if(temp > 0x60)
        {
            temp -= 39;
        }
        temp -= 48; 
        temp *= 16;

        temp += heapBuffer[c];
        if(heapBuffer[c++] > 0x60)
        {
            temp -= 39;
        }
        temp -= 48;
        ptr[i] = temp;
    }
    return b;

}

//------------------------------------------------------------------------------
/**
*/
Util::String
String::AsBase64() const
{
    Util::String ret;
    int allocsize = BASE64_ENCODE_OUT_SIZE(this->strLen) + 1;
    char * buffer = (char *)Memory::Alloc(Memory::ScratchHeap, allocsize);
        
    base64_encode((unsigned char*)this->AsCharPtr(), this->strLen, buffer);
    buffer[allocsize - 1] = '\0';
    ret.SetCharPtr(buffer);
    Memory::Free(Memory::ScratchHeap, buffer);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
String::FromBase64(const String& in)
{
    Util::String ret;
    int allocsize = BASE64_DECODE_OUT_SIZE(in.strLen);
    unsigned char * buffer = (unsigned char *)Memory::Alloc(Memory::ScratchHeap, allocsize);
    
    base64_decode((char*)in.AsCharPtr(), in.strLen, buffer);
    buffer[allocsize - 1] = '\0';
    ret.SetCharPtr((const char*)buffer);
    Memory::Free(Memory::ScratchHeap, buffer);
    return ret;
}


//------------------------------------------------------------------------------
/**
    Allocate a new heap buffer, discards old contents.
*/
void
String::Alloc(SizeT newSize)
{
    n_assert(newSize > (this->strLen + 1));
    n_assert(newSize > this->heapBufferSize);

    // free old buffer
    if (this->heapBuffer)
    {
        Memory::Free(Memory::StringDataHeap, (void*) this->heapBuffer);
        this->heapBuffer = 0;
    }

    // allocate new buffer
    this->heapBuffer = (char*) Memory::Alloc(Memory::StringDataHeap, newSize);
    this->heapBufferSize = newSize;
    this->localBuffer[0] = 0;
}

//------------------------------------------------------------------------------
/**
    (Re-)allocate external buffer and copy existing string contents there.
*/
void
String::Realloc(SizeT newSize)
{
    n_assert(newSize > (this->strLen + 1));
    n_assert(newSize > this->heapBufferSize);

    // allocate a new buffer
    char* newBuffer = (char*) Memory::Alloc(Memory::StringDataHeap, newSize);

    // copy existing contents there...
    if (this->strLen > 0)
    {
        const char* src = this->AsCharPtr();
        Memory::Copy(src, newBuffer, this->strLen);
    }
    newBuffer[this->strLen] = 0;

    // assign new buffer
    if (this->heapBuffer)
    {
        Memory::Free(Memory::StringDataHeap, (void*) this->heapBuffer);
        this->heapBuffer = 0;
    }
    this->localBuffer[0] = 0;
    this->heapBuffer = newBuffer;
    this->heapBufferSize = newSize;
}

//------------------------------------------------------------------------------
/**
    Reserves internal space to prevent excessive heap re-allocations.
    If you plan to do many Append() operations this may help alot.
*/
void
String::Reserve(SizeT newSize)
{
    if (newSize > this->heapBufferSize)
    {
        this->Realloc(newSize);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetByte(byte val)
{
    this->Format("%d", val);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetUByte(ubyte val)
{
    this->Format("%u", val);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetShort(short val)
{
    this->Format("%d", val);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetUShort(ushort val)
{
    this->Format("%u", val);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetInt(int val)
{
    this->Format("%d", val);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetUInt(uint val)
{
    this->Format("%u", val);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetLong(long val)
{
    this->Format("%ld", val);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetSizeT(size_t val)
{
    this->Format("%zu", val);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetLongLong(long long val)
{
    this->Format("%lld", val);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetFloat(float val)
{
    this->Format("%.6f", val);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetDouble(double val)
{
    this->Format("%.6f", val);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetBool(bool val)
{
    if (val)
    {
        this->SetCharPtr("true");
    }
    else
    {
        this->SetCharPtr("false");
    }
}

//------------------------------------------------------------------------------
/**
*/
void
String::Fill(SizeT length, unsigned char character)
{
    this->Alloc(length + 1);
    Memory::Fill(this->heapBuffer, length, character);
    this->heapBuffer[length] = 0;
    this->localBuffer[0] = 0;
    this->strLen = length;
}

#if !__OSX__
//------------------------------------------------------------------------------
/**
*/
void
String::SetVec2(const Math::vec2& v)
{
    this->Format("%.6f,%.6f", v.x, v.y);
}

//------------------------------------------------------------------------------
/**
*/
void 
String::SetVec3(const Math::vec3& v)
{
    this->Format("%.6f,%.6f,%.6f", v.x, v.y, v.z);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetVec4(const Math::vec4& v)
{
    this->Format("%.6f,%.6f,%.6f,%.6f", v.x, v.y, v.z, v.w);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetFloat2(const Math::float2& v)
{
    this->Format("%.6f,%.6f", v.x, v.y);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetFloat3(const Math::float3& v)
{
    this->Format("%.6f,%.6f,%.6f", v.x, v.y, v.z);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetFloat4(const Math::float4& v)
{
    this->Format("%.6f,%.6f,%.6f,%.6f", v.x, v.y, v.z, v.w);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetQuaternion(const Math::quat& v)
{
    this->Format("%.6f,%.6f,%.6f,%.6f", v.x, v.y, v.z, v.w);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetMat4(const Math::mat4& m)
{
    this->Format("%.6f, %.6f, %.6f, %.6f, "
                 "%.6f, %.6f, %.6f, %.6f, "
                 "%.6f, %.6f, %.6f, %.6f, "
                 "%.6f, %.6f, %.6f, %.6f",
                 m.row0.x, m.row0.y, m.row0.z, m.row0.w,
                 m.row1.x, m.row1.y, m.row1.z, m.row1.w,
                 m.row2.x, m.row2.y, m.row2.z, m.row2.w,
                 m.row3.x, m.row3.y, m.row3.z, m.row3.w);
}

//------------------------------------------------------------------------------
/**
*/
void
String::SetTransform44(const Math::transform44& t)
{
    this->Format("%s|%s|%s|%s|%s|%s",
        this->FromVec3(xyz(t.getposition())).AsCharPtr(),
        this->FromQuat(t.getrotate()).AsCharPtr(),
        this->FromVec3(t.getscale()).AsCharPtr(),
        this->FromVec3(xyz(t.getrotatepivot())).AsCharPtr(),
        this->FromVec3(xyz(t.getscalepivot())).AsCharPtr(),
        this->FromMat4(t.getoffset()).AsCharPtr());
}

//------------------------------------------------------------------------------
/**
    Note: this method is not 100% correct, it just checks for invalid characters.
*/
bool
String::IsValidVec2() const
{
    Array<String> tokens(2, 0);
    this->Tokenize(", \t", tokens);
    return this->CheckValidCharSet(" \t-+.,e1234567890") && tokens.Size() == 2;
}

//------------------------------------------------------------------------------
/**
    Note: this method is not 100% correct, it just checks for invalid characters.
*/
bool
String::IsValidVec4() const
{
    Array<String> tokens(4, 0);
    this->Tokenize(", \t", tokens);
    return this->CheckValidCharSet(" \t-+.,e1234567890") && tokens.Size() == 4;
}

//------------------------------------------------------------------------------
/**
    Note: this method is not 100% correct, it just checks for invalid characters.
*/
bool
String::IsValidMat4() const
{
    Array<String> tokens(16, 0);
    this->Tokenize(", \t", tokens);
    return this->CheckValidCharSet(" \t-+.,e1234567890") && tokens.Size() == 16;
}
    
//------------------------------------------------------------------------------
/**
Note: this method is not 100% correct, it just checks for invalid characters.
*/
bool
String::IsValidTransform44() const
{
    Array<String> tokens(6, 0);
    this->Tokenize("|", tokens);
    return this->CheckValidCharSet("| \t-+.,e1234567890") && tokens.Size() == 6;
}

//------------------------------------------------------------------------------
/**
    Returns content as vec2. Note: this method doesn't check whether the
    contents is actually a valid vec4. Use the IsValidFloat2() method
    for this!
*/
Math::vec2
String::AsVec2() const
{
    Array<String> tokens(2, 0);
    this->Tokenize(", \t", tokens);
    n_assert(tokens.Size() == 2);
    Math::vec2 v(tokens[0].AsFloat(), tokens[1].AsFloat());
    return v;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3 
String::AsVec3() const
{
    Array<String> tokens(4, 0);
    this->Tokenize(", \t", tokens);
    n_assert(tokens.Size() == 3);
    Math::vec3 v(tokens[0].AsFloat(), tokens[1].AsFloat(), tokens[2].AsFloat());
    return v;
}

//------------------------------------------------------------------------------
/**
    Returns content as vec4. Note: this method doesn't check whether the
    contents is actually a valid vec4. Use the IsValidVec4() method
    for this!
*/
Math::vec4
String::AsVec4() const
{
    Array<String> tokens(4, 0);
    this->Tokenize(", \t", tokens);
    n_assert(tokens.Size() == 4);
    Math::vec4 v(tokens[0].AsFloat(), tokens[1].AsFloat(), tokens[2].AsFloat(), tokens[3].AsFloat());
    return v;
}

//------------------------------------------------------------------------------
/**
*/
Math::float2
String::AsFloat2() const
{
    Array<String> tokens(4, 0);
    this->Tokenize(", \t", tokens);
    n_assert(tokens.Size() == 4);
    Math::float2 v{ tokens[0].AsFloat(), tokens[1].AsFloat() };
    return v;
}

//------------------------------------------------------------------------------
/**
*/
Math::float3
String::AsFloat3() const
{
    Array<String> tokens(4, 0);
    this->Tokenize(", \t", tokens);
    n_assert(tokens.Size() == 4);
    Math::float3 v{ tokens[0].AsFloat(), tokens[1].AsFloat(), tokens[2].AsFloat() };
    return v;
}

//------------------------------------------------------------------------------
/**
*/
Math::float4
String::AsFloat4() const
{
    Array<String> tokens(4, 0);
    this->Tokenize(", \t", tokens);
    n_assert(tokens.Size() == 4);
    Math::float4 v{ tokens[0].AsFloat(), tokens[1].AsFloat(), tokens[2].AsFloat(), tokens[3].AsFloat() };
    return v;
}
#endif

} // namespace Util
