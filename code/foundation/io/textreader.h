#pragma once
#ifndef IO_TEXTREADER_H
#define IO_TEXTREADER_H
//------------------------------------------------------------------------------
/**
    @class IO::TextReader
    
    A friendly interface for reading text data from a stream.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/streamreader.h"

//------------------------------------------------------------------------------
namespace IO
{
class TextReader : public StreamReader
{
    __DeclareClass(TextReader);
public:
    /// constructor
    TextReader();
    /// destructor
    virtual ~TextReader();
    /// read a single character from the stream
    unsigned char ReadChar();
    /// read until next newline
    Util::String ReadLine();
    /// read entire stream into a string object
    Util::String ReadAll();
    /// read entire stream as lines into string array
    Util::Array<Util::String> ReadAllLines();
};

};
//------------------------------------------------------------------------------
#endif