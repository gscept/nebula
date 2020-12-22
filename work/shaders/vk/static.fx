//------------------------------------------------------------------------------
//  static.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/geometrybase.fxh"
#include "lib/techniques.fxh"

//------------------------------------------------------------------------------
//  Standard methods
//------------------------------------------------------------------------------
SimpleTechnique(
    Static, 
    "Static", 
    vsStatic(), 
    psUber(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = IrradianceOnly
    ),
    StandardState);
    
SimpleTechnique(
    StaticAlphaTest, 
    "Static|AlphaTest", 
    vsStatic(), 
    psUberAlphaTest(
        calcColor = AlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = IrradianceOnly
    ),
    StandardState);
    
SimpleTechnique(
    StaticInstanced, 
    "Static|Instanced", 
    vsStaticInstanced(), 
    psUber(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = IrradianceOnly
    ),
    StandardState);
    
SimpleTechnique(
    StaticInstancedAlphaTest, 
    "Static|Instanced|AlphaTest", 
    vsStaticInstanced(), 
    psUberAlphaTest(
        calcColor = AlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = IrradianceOnly
    ),
    StandardState);

//------------------------------------------------------------------------------
//  IBL + PBR methods
//------------------------------------------------------------------------------
SimpleTechnique(
    StaticEnvironment, 
    "Static|Environment", 
    vsStatic(),
    psUber(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = PBR
    ),
    StandardState);
    
SimpleTechnique(
    StaticEnvironmentAlphaTest, 
    "Static|Environment|AlphaTest", 
    vsStatic(),
    psUberAlphaTest(
        calcColor = AlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = PBR
    ),
    StandardState);
    
SimpleTechnique(
    StaticEnvironmentInstanced, 
    "Static|Environment|Instanced", 
    vsStaticInstanced(),
    psUber(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = PBR
    ),
    StandardState);
    
SimpleTechnique(
    StaticEnvironmentInstancedAlphaTest, 
    "Static|Environment|Instanced|AlphaTest", 
    vsStaticInstanced(),
    psUberAlphaTest(
        calcColor = AlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = PBR
    ),
    StandardState);
    
//------------------------------------------------------------------------------
//  Alpha methods
//------------------------------------------------------------------------------
SimpleTechnique(
    StaticAlpha, 
    "Static|Alpha", 
    vsStatic(), 
    psUber(
        calcColor = AlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = NoEnvironment
    ),
    AlphaState);
    
SimpleTechnique(
    StaticAlphaEnvironment, 
    "Static|Alpha|Environment", 
    vsStatic(), 
    psUber(
        calcColor = AlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = PBR
    ),
    AlphaState);
    
SimpleTechnique(
    StaticAlphaInstanced, 
    "Static|Alpha|Instanced", 
    vsStaticInstanced(), 
    psUber(
        calcColor = AlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = NoEnvironment
    ),
    AlphaState);
    
SimpleTechnique(
    StaticAlphaInstancedEnvironment, 
    "Static|Alpha|Instanced|Environment", 
    vsStaticInstanced(), 
    psUber(
        calcColor = AlphaColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcDepth = ViewSpaceDepthFunctor,
        calcEnv = PBR
    ),
    AlphaState);

TessellationTechnique(
    Tessellated,
    "Static|Tessellated",
    vsStaticTessellated(),
    psUber(
        calcColor = SimpleColor,
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor
    ),
    hsDefault(),
    dsDefault(),
    StandardState
);
