//------------------------------------------------------------------------------
//  @file color.cc
//  @copyright (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "color.h"
namespace Util
{

// All colors should be X11 colors for consistency.

const Color Color::red = Color(0xFF0000FF);
const Color Color::green = Color(0x00FF00FF);
const Color Color::blue = Color(0x0000FFFF);
const Color Color::yellow = Color(0xFFFF00FF);
const Color Color::purple = Color(0xA020F0FF);
const Color Color::orange = Color(0xFFA500FF);
const Color Color::black = Color(0x000000FF);
const Color Color::white = Color(0xFFFFFFFF);
const Color Color::gray = Color(0xBEBEBEFF);

} // namespace Util