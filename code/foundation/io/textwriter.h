#pragma once
#ifndef IO_TEXTWRITER_H
#define IO_TEXTWRITER_H
//------------------------------------------------------------------------------
/**
    @class IO::TextWriter
    
    A friendly interface for writing text data to a stream.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/streamwriter.h"

//------------------------------------------------------------------------------
namespace IO
{
class TextWriter : public StreamWriter
{
    __DeclareClass(TextWriter);
public:
    /// constructor
    TextWriter();
    /// destructor
    virtual ~TextWriter();
    /// write a single character
    void WriteChar(unsigned char c);
    /// write a string
    void WriteString(const Util::String& str);
    /// write some formatted text
    void WriteFormatted(const char* fmtString, ...);
    /// write a line of text and append a new-line
    void WriteLine(const Util::String& line);
    /// write a number of lines, separated by new-lines
    void WriteLines(const Util::Array<Util::String>& lines);
    /// generic writer method
    template<typename T> void Write(const T& t);
};

} // namespace IO
//------------------------------------------------------------------------------
#endif