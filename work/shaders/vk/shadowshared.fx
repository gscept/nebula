//------------------------------------------------------------------------------
//  shadow.fx
//
//  Defines shadows for standard geometry
//
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/skinning.fxh"

#include "lib/shadowbase.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/materialparams.fxh"

//------------------------------------------------------------------------------
/**
*/
// Spotlight methods
SimpleTechnique(Spotlight, "Spot|Static",                               vsStatic(), psShadow(), ShadowState);
SimpleTechnique(SpotlightAlpha, "Spot|Alpha",                           vsStatic(), psShadowAlpha(), ShadowState);
SimpleTechnique(SpotlightSkinned, "Spot|Skinned",                       vsSkinned(), psShadow(), ShadowState);
SimpleTechnique(SpotlightSkinnedAlpha, "Spot|Skinned|Alpha",            vsSkinned(), psShadowAlpha(), ShadowState);
SimpleTechnique(SpotlightInstanced, "Spot|Static|Instanced",            vsStaticInst(), psShadow(), ShadowState);

// Pointlight methods
SimpleTechnique(PointlightDefault, "Point|Static",              vsStaticPoint(), psVSMPoint(), ShadowState);
SimpleTechnique(PointlightAlpha, "Point|Alpha",                 vsStaticPoint(), psVSMAlphaPoint(), ShadowState);
SimpleTechnique(PointlightSkinned, "Point|Skinned",             vsSkinnedPoint(), psVSMPoint(), ShadowState);
SimpleTechnique(PointlightSkinnedAlpha, "Point|Skinned|Alpha",  vsSkinnedPoint(), psVSMAlphaPoint(), ShadowState);
SimpleTechnique(PointlightInstanced, "Point|Static|Instanced",  vsStaticInstPoint(), psVSMPoint(), ShadowState);

// CSM methods
SimpleTechnique(CSM, "Global|Static",                                   vsStaticCSM(), psVSM(), ShadowState);
SimpleTechnique(CSMAlpha, "Global|Alpha",                               vsStaticCSM(), psVSMAlpha(), ShadowState);
SimpleTechnique(CSMInstanced, "Global|Static|Instanced",                vsStaticInstCSM(), psVSM(), ShadowState);
SimpleTechnique(CSMInstancedAlpha, "Global|Alpha|Instanced",            vsStaticInstCSM(), psVSMAlpha(), ShadowState);
SimpleTechnique(CSMSkinned, "Global|Skinned",                           vsSkinnedCSM(), psVSM(), ShadowState);
SimpleTechnique(CSMSkinnedAlpha, "Global|Skinned|Alpha",                vsSkinnedCSM(), psVSMAlpha(), ShadowState);