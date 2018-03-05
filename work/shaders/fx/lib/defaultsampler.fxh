#ifndef DEFAULTSAMPLER_H
#define DEFAULTSAMPLER_H

static const float MipLodBias = -0.5;

SamplerState sourceSampler
{
    AddressU = Clamp;
    AddressV = Clamp;
	Filter = MIN_MAG_MIP_POINT;    
};

SamplerState diffMapSampler
{
    //Texture     = <diffMap>;
    AddressU    = Wrap;
    AddressV    = Wrap;
    Filter = MIN_MAG_MIP_LINEAR;
	MinLOD = 0;
    MipLODBias = MipLodBias;
};

SamplerState bumpMapSampler
{
    //Texture     = <bumpMap>;
    AddressU    = Wrap;
    AddressV    = Wrap;
	Filter = MIN_MAG_MIP_LINEAR;
    MipLODBias = MipLodBias;
};

SamplerState specMapSampler
{
    //Texture     = <specMap>;
    AddressU    = Wrap;
    AddressV    = Wrap;
    Filter = MIN_MAG_MIP_LINEAR;
    MipLODBias = MipLodBias;
};

SamplerState lightMapSampler
{
    //Texture     = <lightMap>;
    AddressU    = Clamp;
    AddressV    = Clamp;
    Filter = MIN_MAG_MIP_LINEAR;
    MipLODBias = MipLodBias;
};

//TextureCube environmentMap : CubeMap0;
SamplerState environmentSampler
{
    //Texture     = <environmentMap>;
    AddressU    = Wrap;
    AddressV    = Wrap;
    Filter = MIN_MAG_MIP_LINEAR;
    MipLODBias = MipLodBias;
};

SamplerState emsvSampler
{
    //Texture     = <emissiveMap>;
    AddressU    = Wrap;
    AddressV    = Wrap;
	Filter = MIN_MAG_LINEAR_MIP_POINT;
    
    MipLODBias = MipLodBias;
};

SamplerState depthSampler
{
    //Texture = <depthTexture>;
    AddressU = Clamp;
    AddressV = Clamp;
	Filter = MIN_MAG_MIP_POINT;
    MipLODBias = 0;
};

SamplerState colorSampler
{
    //Texture = <colorTexture>;
    AddressU = Clamp;
    AddressV = Clamp;
    Filter = MIN_MAG_MIP_LINEAR;
    MipLODBias = 0;
};

SamplerState colorSamplerFiltered
{
    //Texture = <colorTexture>;
    AddressU = Clamp;
    AddressV = Clamp;
	Filter = MIN_MAG_LINEAR_MIP_POINT;
    MipLODBias = 0;
};

SamplerState lightSampler
{
    //Texture = <lightTexture>;
    AddressU = Clamp;
    AddressV = Clamp;
	Filter = MIN_MAG_MIP_POINT;
    MipLODBias = 0;
};

SamplerState alphaLightSampler
{
    //Texture = <alphaLightTexture>;
    AddressU = Clamp;
    AddressV = Clamp;
	Filter = MIN_MAG_MIP_POINT;
    MipLODBias = 0;
};

SamplerState lightSamplerFiltered
{
    //Texture = <lightTexture>;
    AddressU = Clamp;
    AddressV = Clamp;
	Filter = MIN_MAG_MIP_LINEAR;
    MipLODBias = 0;
};

//texture holds view space normal and depth
SamplerState normalMapSampler
{
    //Texture     = <normalMap>;
    AddressU    = Clamp;
    AddressV    = Clamp;
	Filter = MIN_MAG_MIP_POINT;
    MipLODBias = 0;
};

//texture holds view space normal and depth for alpha objects
SamplerState alphaNormalDepthMapSampler
{
    //Texture     = <alphaNormalDepthMapAlpha>;
    AddressU    = Clamp;
    AddressV    = Clamp;
	Filter = MIN_MAG_MIP_POINT;
    MipLODBias = 0;
};

SamplerState dsfObjectDepthSampler
{
    //Texture     = <dsfObjectDepthMap>;
    AddressU    = Clamp;
    AddressV    = Clamp;
	Filter = MIN_MAG_MIP_POINT;
    MipLODBias = 0;
};

//texture holds view space normal and depth
SamplerState alphaDsfObjectIdSampler
{
    //Texture     = <alphaDsfBuffer>;
    AddressU    = Clamp;
    AddressV    = Clamp;
	Filter = MIN_MAG_MIP_POINT;
    MipLODBias = 0;
};

//texture holds view space normal and depth
Texture2D  randomTableMap : RandomTableMap;
SamplerState randomTableSampler 
{
    //Texture     = <randomTableMap>;
    AddressU    = Wrap;
    AddressV    = Wrap;
	Filter = MIN_MAG_MIP_POINT;
    MipLODBias = 0;
};

//texture holds gui
SamplerState guiSampler
{
    //Texture     = <guiBuffer>;
    AddressU    = Clamp;
    AddressV    = Clamp;
 	Filter = MIN_MAG_MIP_POINT;
    MipLODBias = 0;
};

SamplerState SSAOSampler
{
	//Texture = <ssaoBuffer>;
	AddressU = Wrap;
	AddressV = Wrap;
	Filter = MIN_MAG_MIP_LINEAR;
	MipLODBias = MipLodBias;
};


SamplerState pssmShadowBufferSampler
{
    //Texture = <pssmShadowBuffer>;
    AddressU = Clamp;
    AddressV = Clamp;
#if USE_HARDWARE_SAMPLING    
	Filter = MIN_MAG_LINEAR_MIP_POINT;
    
#else    
	Filter = MIN_MAG_MIP_POINT;
    
#endif
	Filter = MIN_MAG_MIP_POINT;
    
};



#endif
