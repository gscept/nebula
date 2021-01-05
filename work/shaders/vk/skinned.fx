//------------------------------------------------------------------------------
//  skinned.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/geometrybase.fxh"
#include "lib/standard_shading.fxh"
#include "lib/techniques.fxh"
#include "lib/skinning.fxh"

//------------------------------------------------------------------------------
//  Depth prepass methods
//------------------------------------------------------------------------------
SimpleTechnique(SkinnedDepth, "Skinned|Depth", vsDepthSkinned(), psDepthOnly(), DepthState);
SimpleTechnique(SkinnedDepthDoubleSided, "Skinned|Depth|DoubleSided", vsDepthSkinned(), psDepthOnly(), DepthStateDoubleSided);
SimpleTechnique(SkinnedDepthAlphaMask, "Skinned|Depth|AlphaMask", vsDepthSkinnedAlphaMask(), psDepthOnlyAlphaMask(), DepthState);
SimpleTechnique(SkinnedDepthAlphaMaskDoubleSided, "Skinned|Depth|AlphaMask|DoubleSided", vsDepthSkinnedAlphaMask(), psDepthOnlyAlphaMask(), DepthStateDoubleSided);

//------------------------------------------------------------------------------
//  Standard methods
//------------------------------------------------------------------------------
SimpleTechnique(
    Skinned, 
    "Skinned", 
    vsSkinned(), 
    psStandard(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = NoEnvironment,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);
    
SimpleTechnique(
    SkinnedAlphaTest, 
    "Skinned|AlphaTest", 
    vsSkinned(), 
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
    SkinnedEnvironment, 
    "Skinned|Environment", 
    vsSkinned(),
    psStandard(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);
    
SimpleTechnique(
    SkinnedEnvironmentAlphaTest, 
    "Skinned|Environment|AlphaTest", 
    vsSkinned(),
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
    SkinnedAlpha, 
    "Skinned|Alpha", 
    vsSkinned(), 
    psStandard(
        calcColor = AlphaMaskSimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = NoEnvironment,
        finalizeColor = FinalizeAlpha
    ),
    AlphaState);
    
SimpleTechnique(
    SkinnedAlphaEnvironment, 
    "Skinned|Alpha|Environment", 
    vsSkinned(), 
    psStandard(
        calcColor = AlphaMaskSimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeAlpha
    ),
    AlphaState);
