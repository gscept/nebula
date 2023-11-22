#pragma once
//------------------------------------------------------------------------------
/**
    Helper type to create colored and formatted text

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/string.h"
namespace ToolkitUtil
{

enum class TextColor
{
    Default,
    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White
};

enum class FontMode
{
    Normal,
    Bold,
    Dim,
    Italic,
    Underline,
    Blink,
    Inverse,
    Hidden,
    StrikeThrough
};

class Text
{
public:
    /// Constructor
    Text(const char* string, size_t size);
    /// Constructor from string
    Text(const Util::String& string);
    /// Destructor
    ~Text();

    /// Set color
    Text& Color(const TextColor color);
    /// Set font mode
    Text& Style(const FontMode mode);

    /// Move cursor up
    Text& CursorUp(SizeT lines);
    /// Move cursor down
    Text& CursorDown(SizeT lines);
    /// Move cursor to position
    Text& CursorMove(SizeT line);
    /// Erase current line
    Text& CursorEraseLine();

    /// Get string
    const Util::String AsString();
    /// Get as char ptr
    const char* AsCharPtr();

private:

    Util::String format;
    Util::String cursor;
    Util::String commands;
    Util::String string;

    Util::String formattedString;
};

extern Text operator""_text(const char* str, size_t size);

#if defined(NEBULA_TEXT_IMPLEMENT)
const char* ColorStrings[] =
{
    "39"
    , "90"
    , "91"
    , "92"
    , "93"
    , "94"
    , "95"
    , "96"
    , "97"
};

const char* FontModeStrings[] =
{
    "-1"
    , "1"
    , "2"
    , "3"
    , "4"
    , "5"
    , "7"
    , "8"
    , "9"
};

//------------------------------------------------------------------------------
/**
*/
Text::Text(const char* string, size_t size)
{
    this->string.Set(string, size);
}

//------------------------------------------------------------------------------
/**
*/
Text::Text(const Util::String& string)
{
    this->string = string;
}

//------------------------------------------------------------------------------
/**
*/
Text::~Text()
{
    // Do nothing
}

//------------------------------------------------------------------------------
/**
*/
Text&
Text::Color(const TextColor color)
{
    if (!this->format.IsEmpty())
        this->format += ";";
    this->format.Append(ColorStrings[(uint)color]);
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
Text& 
Text::Style(const FontMode mode)
{
    if (!this->format.IsEmpty())
        this->format += ";";
    this->format.Append(FontModeStrings[(uint)mode]);
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
const 
Util::String Text::AsString()
{
    return Util::String::Sprintf("\033[%sm%s%s\033[0m", this->format.AsCharPtr(), this->cursor.AsCharPtr(), this->string.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
const char* 
Text::AsCharPtr()
{
    this->formattedString = this->AsString();
    return this->formattedString.AsCharPtr();
}

//------------------------------------------------------------------------------
/**
*/
Text 
operator""_text(const char* str, size_t size)
{
    return Text(str, size);
}

#endif

} // namespace ToolkitUtil
