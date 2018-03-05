//------------------------------------------------------------------------------
//  skinned.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/geometrybase.fxh"

SimpleTechnique(Skinned, "Skinned", vsSkinned(), psDefault(), SolidBlend, SolidDepthStencil, Rasterizer)
SimpleTechnique(Environment, "Skinned|Environment", vsSkinned(), psEnvironment(), SolidBlend, SolidDepthStencil, Rasterizer)
SimpleTechnique(Alpha, "Skinned|Alpha", vsSkinned(), psAlpha(), AlphaBlend, AlphaDepthStencil, Rasterizer)
SimpleTechnique(AlphaEnvironment, "Skinned|Alpha|Environment", vsSkinned(), psEnvironment(), AlphaBlend, AlphaDepthStencil, Rasterizer)
TessellationTechnique(Tessellated, "Skinned|Tessellated", vsSkinnedTessellated(), psDefault(), hsDefault(), dsDefault(), SolidBlend, SolidDepthStencil, Rasterizer)
TessellationTechnique(TessellatedEnvironment, "Skinned|Tessellated|Environment", vsSkinnedTessellated(), psEnvironment(), hsDefault(), dsDefault(), SolidBlend, SolidDepthStencil, Rasterizer)
