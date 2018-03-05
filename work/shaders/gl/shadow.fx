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


//------------------------------------------------------------------------------
/**
*/
// Spotlight methods
SimpleTechnique(Spotlight, "Spot|Static", 								vsStatic(), psShadow(), ShadowState);
SimpleTechnique(SpotlightAlpha, "Spot|Alpha", 							vsStatic(), psShadowAlphaTest(), ShadowState);
SimpleTechnique(SpotlightSkinned, "Spot|Skinned", 						vsSkinned(), psShadow(), ShadowState);
SimpleTechnique(SpotlightSkinnedAlpha, "Spot|Skinned|Alpha", 			vsSkinned(), psShadowAlphaTest(), ShadowState);
SimpleTechnique(SpotlightInstanced, "Spot|Static|Instanced", 			vsStaticInst(), psShadow(), ShadowState);
TessellationTechnique(SpotlightTessellated, "Spot|Static|Tessellated", 	vsTess(), psShadow(), hsShadow(), dsShadow(), ShadowState);

// Pointlight methods
GeometryTechnique(PointlightDefault, "Point|Static", 				vsStaticPoint(), PS_METHOD_STANDARD, gsPoint(), ShadowState);
GeometryTechnique(PointlightAlpha, "Point|Alpha", 					vsStaticPoint(), PS_METHOD_ALPHA, gsPoint(), ShadowState);
GeometryTechnique(PointlightSkinned, "Point|Skinned", 				vsSkinnedPoint(), PS_METHOD_STANDARD, gsPoint(), ShadowState);
GeometryTechnique(PointlightSkinnedAlpha, "Point|Skinned|Alpha", 	vsSkinnedPoint(), PS_METHOD_ALPHA, gsPoint(), ShadowState);
GeometryTechnique(PointlightInstanced, "Point|Static|Instanced", 	vsStaticInstPoint(), PS_METHOD_STANDARD, gsPoint(), ShadowState);
//FullTechnique(PointlightTessellated, "Point|Static|Tessellated", 	vsTessCSM(), psVSM(), hsShadow(), dsCSM(), gsPoint(), ShadowState);

// CSM methods
GeometryTechnique(CSM, "Global|Static", 							vsStaticCSM(), PS_METHOD_STANDARD, gsCSM(), ShadowStateCSM);
GeometryTechnique(CSMAlphaTest, "Global|AlphaTest", 				vsStaticCSM(), PS_METHOD_ALPHA_TEST, gsCSM(), ShadowStateCSM);
GeometryTechnique(CSMTrans, "Global|Alpha", 						vsStaticCSM(), PS_METHOD_ALPHA, gsCSM(), ShadowStateCSMAlpha);
GeometryTechnique(CSMInstanced, "Global|Static|Instanced", 			vsStaticInstCSM(), PS_METHOD_STANDARD, gsCSM(), ShadowStateCSM);
GeometryTechnique(CSMInstancedAlpha, "Global|AlphaTest|Instanced", 	vsStaticInstCSM(), PS_METHOD_ALPHA_TEST, gsCSM(), ShadowStateCSM);
GeometryTechnique(CSMSkinned, "Global|Skinned", 					vsSkinnedCSM(), PS_METHOD_STANDARD, gsCSM(), ShadowStateCSM);
GeometryTechnique(CSMSkinnedAlpha, "Global|Skinned|AlphaTest", 		vsSkinnedCSM(), PS_METHOD_ALPHA_TEST, gsCSM(), ShadowStateCSM);
FullTechnique(CSMTessellated, "Global|Static|Tessellated", 			vsTessCSM(), PS_METHOD_STANDARD, hsShadow(), dsCSM(), gsCSM(), ShadowStateCSM);