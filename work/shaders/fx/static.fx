//------------------------------------------------------------------------------
//  static.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/geometrybase.fxh"

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Static, "Static", vsStatic(), psDefault(), SolidBlend, SolidDepthStencil, Rasterizer)
SimpleTechnique(StaticInstanced, "Static|Instanced", vsStaticInstanced(), psDefault(), SolidBlend, SolidDepthStencil, Rasterizer)
SimpleTechnique(Environment, "Static|Environment", vsStatic(), psEnvironment(), SolidBlend, SolidDepthStencil, Rasterizer)
SimpleTechnique(Alpha, "Alpha", vsStatic(), psAlpha(), AlphaBlend, AlphaDepthStencil, Rasterizer)
SimpleTechnique(AlphaInstanced, "Alpha|Instanced", vsStaticInstanced(), psAlpha(), AlphaBlend, AlphaDepthStencil, Rasterizer)
TessellationTechnique(Tessellated, "Static|Tessellated", vsStaticTessellated(), psDefault(), hsDefault(), dsDefault(), SolidBlend, SolidDepthStencil, Rasterizer)
