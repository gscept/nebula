//------------------------------------------------------------------------------
//  defaultsamplers.fxh
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef DEFAULTSAMPLERS_FXH
#define DEFAULTSAMPLERS_FXH


group(INSTANCE_GROUP) shared varblock DefaultSamplers [ bool DynamicOffset = true; string Visibility = "PS"; ]
{
	textureHandle AlbedoMap;
	textureHandle DisplacementMap;
	textureHandle SpecularMap;
	textureHandle EmissiveMap;
	textureHandle NormalMap;
	textureHandle RoughnessMap;
	textureHandle CavityMap;
	
	// textures
	int NumEnvMips = 9;
	textureHandle EnvironmentMap;
	textureHandle IrradianceMap;
};


// samplers
samplerstate GeometryTextureSampler
{
	// Samplers = { SpecularMap, EmissiveMap, NormalMap, AlbedoMap, DisplacementMap, RoughnessMap, CavityMap };
	//Filter = MinMagMipLinear;
	//AddressU = Wrap;
	//AddressV = Wrap;
};

samplerstate MaterialSampler
{

};

samplerstate CubeSampler
{
	Filter = MinMagMipLinear;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
};

samplerstate EnvironmentSampler
{
	//Samplers = { EnvironmentMap, IrradianceMap };
	Filter = MinMagMipLinear;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
};

#endif // DEFAULTSAMPLERS_FXH

