//------------------------------------------------------------------------------
//  boxtapvert.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#define Vert
#define Taps7
#include "lib/std.fxh"
#include "lib/boxtap.fxh"
#include "lib/techniques.fxh"

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), BoxtapState);