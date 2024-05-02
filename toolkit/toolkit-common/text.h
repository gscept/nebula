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

} // namespace ToolkitUtil
