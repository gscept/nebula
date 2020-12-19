//------------------------------------------------------------------------------
//  skinned.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/geometrybase.fxh"
#include "lib/techniques.fxh"
#include "lib/skinning.fxh"

//------------------------------------------------------------------------------
//  Standard methods
//------------------------------------------------------------------------------
SimpleTechnique(
    Skinned, 
    "Skinned", 
    vsSkinned(), 
    psUber(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor, 
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = PBR
    ),
    StandardState);
    
SimpleTechnique(
    SkinnedAlphaTest, 
    "Skinned|AlphaTest", 
    vsSkinned(), 
    psUberAlphaTest(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = PBR
    ),
    StandardState);
    
//------------------------------------------------------------------------------
//  IBL + PBR methods
//------------------------------------------------------------------------------
/*
SimpleTechnique(
    SkinnedEnvironment, 
    "Skinned|Environment", 
    vsSkinned(),
    psUber(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = PBR
    ),
    StandardState);
    
SimpleTechnique(
    SkinnedEnvironmentAlphaTest, 
    "Skinned|Environment|AlphaTest", 
    vsSkinned(),
    psUberAlphaTest(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = PBR
    ),
    StandardState);
*/
//------------------------------------------------------------------------------
//  Alpha methods
//------------------------------------------------------------------------------
SimpleTechnique(
    SkinnedAlpha, 
    "Skinned|Alpha", 
    vsSkinned(), 
    psUber(
        calcColor = AlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = PBR 
    ),
    AlphaState);
    
/*
SimpleTechnique(
    SkinnedAlphaEnvironment, 
    "Skinned|Alpha|Environment", 
    vsSkinned(), 
    psUber(
        calcColor = AlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = PBR
    ),
    AlphaState);
*/

    /*
SimpleTechnique(Skinned, "Skinned", vsSkinned(), psUber(), StandardState);
SimpleTechnique(Environment, "Skinned|Environment", vsSkinned(), psUber(), StandardState);
SimpleTechnique(Alpha, "Skinned|Alpha", vsSkinned(), psUber(), AlphaState);
SimpleTechnique(AlphaEnvironment, "Skinned|Alpha|Environment", vsSkinned(), psUber(), AlphaState);
*/
TessellationTechnique(
    Tessellated,
    "Skinned|Tessellated",
    vsSkinnedTessellated(),
    psUber(
        calcColor = AlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor
    ),
    hsDefault(),
    dsDefault(),
    StandardState
);
TessellationTechnique(
    TessellatedEnvironment,
    "Skinned|Tessellated|Environment",
    vsSkinnedTessellated(),
    psUber(
        calcColor = AlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor
    ),
    hsDefault(),
    dsDefault(),
    StandardState
);

//TransformFeedbackTechnique(SkinnedFeedback, "Skinned|Alt0", vsTransformSkinned());
