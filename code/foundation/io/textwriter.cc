//------------------------------------------------------------------------------
//  textwriter.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "io/textwriter.h"

namespace IO
{
__ImplementClass(IO::TextWriter, 'TXTW', IO::StreamWriter);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
TextWriter::TextWriter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TextWriter::~TextWriter()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TextWriter::WriteChar(unsigned char c)
{
    this->stream->Write(&c, sizeof(c));
}

//------------------------------------------------------------------------------
/**
*/
void
TextWriter::WriteString(const String& str)
{
    this->stream->Write(str.AsCharPtr(), str.Length());
}

//------------------------------------------------------------------------------
/**
*/
void __cdecl
TextWriter::WriteFormatted(const char* fmtString, ...)
{
    va_list argList;
    va_start(argList, fmtString);
    String str;
    str.FormatArgList(fmtString, argList);
    this->stream->Write(str.AsCharPtr(), str.Length());
    va_end(argList);
}

//------------------------------------------------------------------------------
/**
*/
void
TextWriter::WriteLine(const String& line)
{
    this->WriteString(line);
    this->WriteChar('\n');
}

//------------------------------------------------------------------------------
/**
*/
void
TextWriter::WriteLines(const Array<String>& lines)
{
    IndexT i;
    for (i = 0; i < lines.Size(); i++)
    {
        this->WriteLine(lines[i]);
    }
}

} // namespace IO