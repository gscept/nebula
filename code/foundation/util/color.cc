//------------------------------------------------------------------------------
//  @file color.cc
//  @copyright (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "color.h"
namespace Util
{

// All colors should be X11 colors for consistency.

const Color Color::red = Color(0xFFFF0000);
const Color Color::green = Color(0xFF00FF00);
const Color Color::blue = Color(0xFF0000FF);
const Color Color::yellow = Color(0xFFFFFF00);
const Color Color::purple = Color(0xFFA020F0);
const Color Color::orange = Color(0xFFFFA500);
const Color Color::black = Color(0xFF000000);
const Color Color::white = Color(0xFFFFFFFF);
const Color Color::gray = Color(0xFFBEBEBE);

} // namespace Util