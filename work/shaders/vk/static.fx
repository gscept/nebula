//------------------------------------------------------------------------------
//  static.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/geometrybase.fxh"
#include "lib/standard_shading.fxh"
#include "lib/techniques.fxh"

//------------------------------------------------------------------------------
//  Depth prepass methods
//------------------------------------------------------------------------------
SimpleTechnique(StaticDepth, "Static|Depth", vsDepthStatic(), psDepthOnly(), DepthState);
SimpleTechnique(StaticDepthDoubleSided, "Static|Depth|DoubleSided", vsDepthStatic(), psDepthOnly(), DepthStateDoubleSided);
SimpleTechnique(StaticDepthAlphaMask, "Static|Depth|AlphaMask", vsDepthStaticAlphaMask(), psDepthOnlyAlphaMask(), DepthState);
SimpleTechnique(StaticDepthAlphaMaskDoubleSided, "Static|Depth|AlphaMask|DoubleSided", vsDepthStaticAlphaMask(), psDepthOnlyAlphaMask(), DepthStateDoubleSided);

//------------------------------------------------------------------------------
//  Standard methods
//------------------------------------------------------------------------------
SimpleTechnique(
    Static, 
    "Static", 
    vsStatic(), 
    psStandard(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = NoEnvironment,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);
    
SimpleTechnique(
    StaticAlphaMask,
    "Static|AlphaMask", 
    vsStatic(), 
    psStandard(
        calcColor = AlphaMaskSimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = NoEnvironment,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);
    
SimpleTechnique(
    StaticInstanced, 
    "Static|Instanced", 
    vsStaticInstanced(), 
    psStandard(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = NoEnvironment,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);
    
SimpleTechnique(
    StaticInstancedAlphaMask, 
    "Static|Instanced|AlphaMask", 
    vsStaticInstanced(), 
    psStandard(
        calcColor = AlphaMaskSimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = NoEnvironment,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);

//------------------------------------------------------------------------------
//  IBL + PBR methods
//------------------------------------------------------------------------------
SimpleTechnique(
    StaticEnvironment, 
    "Static|Environment", 
    vsStatic(),
    psStandard(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);
    
SimpleTechnique(
    StaticEnvironmentAlphaMask, 
    "Static|Environment|AlphaMask", 
    vsStatic(),
    psStandard(
        calcColor = AlphaMaskSimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);
    
SimpleTechnique(
    StaticEnvironmentInstanced, 
    "Static|Environment|Instanced", 
    vsStaticInstanced(),
    psStandard(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);
    
SimpleTechnique(
    StaticEnvironmentInstancedAlphaMask, 
    "Static|Environment|Instanced|AlphaMask", 
    vsStaticInstanced(),
    psStandard(
        calcColor = AlphaMaskSimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);
    
//------------------------------------------------------------------------------
//  Alpha methods
//------------------------------------------------------------------------------
SimpleTechnique(
    StaticAlpha, 
    "Static|Alpha", 
    vsStatic(), 
    psStandard(
        calcColor = AlphaMaskAlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = NoEnvironment,
        finalizeColor = FinalizeAlpha
    ),
    AlphaState);
    
SimpleTechnique(
    StaticAlphaEnvironment, 
    "Static|Alpha|Environment", 
    vsStatic(), 
    psStandard(
        calcColor = AlphaMaskAlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeAlpha
    ),
    AlphaState);
    
SimpleTechnique(
    StaticAlphaInstanced, 
    "Static|Alpha|Instanced", 
    vsStaticInstanced(), 
    psStandard(
        calcColor = AlphaMaskAlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = NoEnvironment,
        finalizeColor = FinalizeAlpha
    ),
    AlphaState);
    
SimpleTechnique(
    StaticAlphaInstancedEnvironment, 
    "Static|Alpha|Instanced|Environment", 
    vsStaticInstanced(), 
    psStandard(
        calcColor = AlphaMaskAlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeAlpha
    ),
    AlphaState);
