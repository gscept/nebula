//------------------------------------------------------------------------------
//  defaultsamplers.fxh
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef DEFAULTSAMPLERS_FXH
#define DEFAULTSAMPLERS_FXH

// samplers
sampler_state GeometryTextureSampler
{
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
	Filter = MinMagMipLinear;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
};

#endif // DEFAULTSAMPLERS_FXH

