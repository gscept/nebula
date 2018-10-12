//------------------------------------------------------------------------------
//  textreader.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/textreader.h"
#include <string.h>

namespace IO
{
__ImplementClass(IO::TextReader, 'TXTR', IO::StreamReader);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
TextReader::TextReader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TextReader::~TextReader()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
unsigned char
TextReader::ReadChar()
{
    unsigned char c;
    this->stream->Read(&c, sizeof(c));
    return c;
}

//------------------------------------------------------------------------------
/**
*/
String
TextReader::ReadLine()
{
    // read chunk until newline found
    String result;
    char tmp[2];
    tmp[1] = '\0';
    while(!this->stream->Eof())
    {
        if(1 != this->stream->Read(tmp, 1)) break;
        if(tmp[0] == '\n') break;
        result.Append(String(tmp));
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
TextReader::ReadAll()
{
    String result;
    if (this->stream->GetSize() > 0)
    {
        this->stream->Seek(0, Stream::Begin);
        Stream::Size size = this->stream->GetSize();
        char* buf = (char*) Memory::Alloc(Memory::ScratchHeap, size + 1);
        Stream::Size bytesRead = this->stream->Read(buf, size);
        buf[bytesRead] = 0;
        result = buf;
        Memory::Free(Memory::ScratchHeap, buf);
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Array<String>
TextReader::ReadAllLines()
{
    Array<String> result;
    while (!stream->Eof())
    {
        result.Append(this->ReadLine());
    }
    return result;
}

} // namespace IO
