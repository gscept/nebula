//------------------------------------------------------------------------------
//  defaultsamplers.fxh
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef DEFAULTSAMPLERS_FXH
#define DEFAULTSAMPLERS_FXH

group(BATCH_GROUP) shared varblock DefaultSamplers [ string Visibility = "VS|PS"; ]
{
	textureHandle AlbedoMap;
	textureHandle DisplacementMap;
	textureHandle ParameterMap;
	textureHandle NormalMap;
};


// samplers
samplerstate GeometryTextureSampler
{
	// Samplers = { ParameterMap, EmissiveMap, NormalMap, AlbedoMap, DisplacementMap, RoughnessMap, CavityMap };
	//Filter = MinMagMipLinear;
	//AddressU = Wrap;
	//AddressV = Wrap;
};

samplerstate MaterialSampler
{

};

samplerstate NormalSampler
{
	Filter = MinMagLinearMipPoint;
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

