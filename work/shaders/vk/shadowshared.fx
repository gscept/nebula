//------------------------------------------------------------------------------
//  shadow.fx
//
//	Defines shadows for standard geometry
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
SimpleTechnique(Spotlight, "Spot|Static", 								vsStatic(), psShadow(), ShadowState);
SimpleTechnique(SpotlightAlpha, "Spot|Alpha", 							vsStatic(), psShadowAlpha(), ShadowState);
SimpleTechnique(SpotlightSkinned, "Spot|Skinned", 						vsSkinned(), psShadow(), ShadowState);
SimpleTechnique(SpotlightSkinnedAlpha, "Spot|Skinned|Alpha", 			vsSkinned(), psShadowAlpha(), ShadowState);
SimpleTechnique(SpotlightInstanced, "Spot|Static|Instanced", 			vsStaticInst(), psShadow(), ShadowState);
TessellationTechnique(SpotlightTessellated, "Spot|Static|Tessellated", 	vsTess(), psShadow(), hsShadow(), dsShadow(), ShadowState);

// Pointlight methods
GeometryTechnique(PointlightDefault, "Point|Static", 				vsStaticPoint(), psVSMPoint(), gsPoint(), ShadowState);
GeometryTechnique(PointlightAlpha, "Point|Alpha", 					vsStaticPoint(), psVSMAlphaPoint(), gsPoint(), ShadowState);
GeometryTechnique(PointlightSkinned, "Point|Skinned", 				vsSkinnedPoint(), psVSMPoint(), gsPoint(), ShadowState);
GeometryTechnique(PointlightSkinnedAlpha, "Point|Skinned|Alpha", 	vsSkinnedPoint(), psVSMAlphaPoint(), gsPoint(), ShadowState);
GeometryTechnique(PointlightInstanced, "Point|Static|Instanced", 	vsStaticInstPoint(), psVSMPoint(), gsPoint(), ShadowState);
//FullTechnique(PointlightTessellated, "Point|Static|Tessellated", 	vsTessCSM(), psVSMPoint(), hsShadow(), dsCSM(), gsPoint(), ShadowState);

// CSM methods
GeometryTechnique(CSM, "Global|Static", 							vsStaticCSM(), psVSM(), gsCSM(), ShadowStateCSM);
GeometryTechnique(CSMAlpha, "Global|Alpha", 						vsStaticCSM(), psVSMAlpha(), gsCSM(), ShadowStateCSM);
GeometryTechnique(CSMInstanced, "Global|Static|Instanced", 			vsStaticInstCSM(), psVSM(), gsCSM(), ShadowStateCSM);
GeometryTechnique(CSMInstancedAlpha, "Global|Alpha|Instanced", 		vsStaticInstCSM(), psVSMAlpha(), gsCSM(), ShadowStateCSM);
GeometryTechnique(CSMSkinned, "Global|Skinned", 					vsSkinnedCSM(), psVSM(), gsCSM(), ShadowStateCSM);
GeometryTechnique(CSMSkinnedAlpha, "Global|Skinned|Alpha", 			vsSkinnedCSM(), psVSMAlpha(), gsCSM(), ShadowStateCSM);
FullTechnique(CSMTessellated, "Global|Static|Tessellated", 			vsTessCSM(), psVSM(), hsShadow(), dsCSM(), gsCSM(), ShadowStateCSM);