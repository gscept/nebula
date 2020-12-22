//------------------------------------------------------------------------------
//  shadow.fx
//
//  Defines shadows for standard geometry
//
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/skinning.fxh"
#include "lib/shadowbase.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/materialparams.fxh"
#include "lib/tessellationparams.fxh"

//------------------------------------------------------------------------------
/**
*/
// Spotlight methods
SimpleTechnique(Spotlight, "Spot|Static",                               vsStatic(), psShadow(), ShadowState);
SimpleTechnique(SpotlightAlpha, "Spot|Alpha",                           vsStatic(), psShadowAlpha(), ShadowState);
SimpleTechnique(SpotlightSkinned, "Spot|Skinned",                       vsSkinned(), psShadow(), ShadowState);
SimpleTechnique(SpotlightSkinnedAlpha, "Spot|Skinned|Alpha",            vsSkinned(), psShadowAlpha(), ShadowState);
SimpleTechnique(SpotlightInstanced, "Spot|Static|Instanced",            vsStaticInst(), psShadow(), ShadowState);
TessellationTechnique(SpotlightTessellated, "Spot|Static|Tessellated",  vsTess(), psShadow(), hsShadow(), dsShadow(), ShadowState);

// Pointlight methods
SimpleTechnique(PointlightDefault, "Point|Static",              vsStaticPoint(), psVSMPoint(), ShadowState);
SimpleTechnique(PointlightAlpha, "Point|Alpha",                 vsStaticPoint(), psVSMAlphaPoint(), ShadowState);
SimpleTechnique(PointlightSkinned, "Point|Skinned",             vsSkinnedPoint(), psVSMPoint(), ShadowState);
SimpleTechnique(PointlightSkinnedAlpha, "Point|Skinned|Alpha",  vsSkinnedPoint(), psVSMAlphaPoint(), ShadowState);
SimpleTechnique(PointlightInstanced, "Point|Static|Instanced",  vsStaticInstPoint(), psVSMPoint(), ShadowState);
//FullTechnique(PointlightTessellated, "Point|Static|Tessellated",  vsTessCSM(), psVSMPoint(), hsShadow(), dsCSM(), ShadowState);

// CSM methods
SimpleTechnique(CSM, "Global|Static",                               vsStaticCSM(), psVSM(), ShadowStateCSM);
SimpleTechnique(CSMAlpha, "Global|Alpha",                           vsStaticCSM(), psVSMAlpha(), ShadowStateCSM);
SimpleTechnique(CSMInstanced, "Global|Static|Instanced",            vsStaticInstCSM(), psVSM(), ShadowStateCSM);
SimpleTechnique(CSMInstancedAlpha, "Global|Alpha|Instanced",        vsStaticInstCSM(), psVSMAlpha(), ShadowStateCSM);
SimpleTechnique(CSMSkinned, "Global|Skinned",                       vsSkinnedCSM(), psVSM(), ShadowStateCSM);
SimpleTechnique(CSMSkinnedAlpha, "Global|Skinned|Alpha",            vsSkinnedCSM(), psVSMAlpha(), ShadowStateCSM);
TessellationTechnique(CSMTessellated, "Global|Static|Tessellated",  vsTessCSM(), psVSM(), hsShadow(), dsCSM(), ShadowStateCSM);