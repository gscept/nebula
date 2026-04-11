//------------------------------------------------------------------------------
//  @file text.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "text.h"
namespace ToolkitUtil
{

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

bool disableTextColors = false;

//------------------------------------------------------------------------------
/**
*/
Text::Text(const char* string, size_t size)
{
    this->string.Set(string, static_cast<SizeT>(size));
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
    if (disableTextColors)
        return *this;

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
    if (disableTextColors)
        return *this;

    if (!this->format.IsEmpty())
        this->format += ";";
    this->format.Append(FontModeStrings[(uint)mode]);
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String 
Text::AsString()
{
    if (disableTextColors)
        return this->string;

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


} // namespace ToolkitUtil
