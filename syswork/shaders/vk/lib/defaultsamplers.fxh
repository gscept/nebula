//------------------------------------------------------------------------------
//  defaultsamplers.fxh
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef DEFAULTSAMPLERS_FXH
#define DEFAULTSAMPLERS_FXH

group(BATCH_GROUP) shared constant DefaultSamplers [ string Visibility = "VS|PS"; ]
{
	textureHandle AlbedoMap;
	textureHandle DisplacementMap;
	textureHandle ParameterMap;
	textureHandle NormalMap;
};


// samplers
sampler_state GeometryTextureSampler
{
	// Samplers = { ParameterMap, EmissiveMap, NormalMap, AlbedoMap, DisplacementMap, RoughnessMap, CavityMap };
	//Filter = MinMagMipLinear;
	//AddressU = Wrap;
	//AddressV = Wrap;
};

sampler_state MaterialSampler
{
	Filter = MinMagMipLinear;
};

sampler_state NormalSampler
{
	Filter = MinMagLinearMipPoint;
};

sampler_state CubeSampler
{
	Filter = MinMagMipLinear;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
};

sampler_state EnvironmentSampler
{
	//Samplers = { EnvironmentMap, IrradianceMap };
	Filter = MinMagMipLinear;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
};

#endif // DEFAULTSAMPLERS_FXH

