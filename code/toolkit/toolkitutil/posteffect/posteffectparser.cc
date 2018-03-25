//------------------------------------------------------------------------------
//  posteffectparser.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "posteffectparser.h"
#include "io/ioserver.h"
#include "io/xmlreader.h"
#include "io/xmlwriter.h"

using namespace IO;
using namespace Util;

namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
bool
PostEffectParser::Load(const Util::String & path, PostEffect::PostEffectEntity::ParamSet & parms)
{
	Ptr<Stream> fxStream = IoServer::Instance()->CreateStream(path);
	fxStream->SetAccessMode(IO::Stream::ReadAccess);
	Ptr<XmlReader> reader = XmlReader::Create();

	if (!fxStream->Open())
	{
		return false;
	}

	reader->SetStream(fxStream);

	if (!reader->Open())
	{
		return false;
	}

	if (!reader->HasNode("/PostFX"))
	{
		return false;
	}

	reader->SetToNode("/PostFX");

	parms.light->SetLightTransform(reader->GetMatrix44("GlobalLightTransform"));
	parms.light->SetLightColor(reader->GetFloat4("GlobalLightDiffuse"));
	parms.light->SetLightOppositeColor(reader->GetFloat4("GlobalLightOpposite"));
	parms.light->SetLightAmbientColor(reader->GetFloat4("GlobalLightAmbient"));
	parms.light->SetLightCastShadows(reader->GetBool("GlobalLightCastShadows"));
	parms.light->SetLightShadowIntensity(reader->GetFloat("GlobalLightShadowIntensity"));
	parms.light->SetLightIntensity(reader->GetFloat("GlobalLightIntensity"));
	parms.light->SetBackLightFactor(reader->GetFloat("GlobalLightBacklightFactor"));
	parms.light->SetLightShadowBias(reader->GetFloat("GlobalLightShadowBias"));

	parms.color->SetColorSaturation(reader->GetFloat("Saturation"));
	parms.color->SetColorBalance(reader->GetFloat4("Balance"));
	parms.color->SetColorMaxLuminance(reader->GetFloat("MaxLuminance"));

	parms.hdr->SetHdrBloomIntensity(reader->GetFloat("BloomScale"));
	parms.hdr->SetHdrBloomColor(reader->GetFloat4("BloomColor"));
	parms.hdr->SetHdrBloomThreshold(reader->GetFloat("BloomThreshold"));

	parms.fog->SetFogNearDistance(reader->GetFloat("FogNearDist"));
	parms.fog->SetFogFarDistance(reader->GetFloat("FogFarDist"));
	parms.fog->SetFogHeight(reader->GetFloat("FogHeight"));
	parms.fog->SetFogColorAndIntensity(reader->GetFloat4("FogColor"));

	parms.dof->SetFocusDistance(reader->GetFloat("FocusDistance"));
	parms.dof->SetFocusLength(reader->GetFloat("FocusLength"));
	parms.dof->SetFilterSize(reader->GetFloat("FocusRadius"));

	parms.sky->SetSkyContrast(reader->GetFloat("SkyContrast"));
	parms.sky->SetSkyBrightness(reader->GetFloat("SkyBrightness"));
    parms.sky->SetSkyRotationFactor(reader->GetOptFloat("SkyRotationFactor", 0.03f));
	parms.sky->SetSkyTexturePath(reader->GetString("SkyTexture"));
    parms.sky->SetReflectanceTexturePath(reader->GetOptString("ProbeReflectionMap", "tex:system/sky_refl"));
    parms.sky->SetIrradianceTexturePath(reader->GetOptString("ProbeIrradianceMap", "tex:system/sky_irr"));
    


	parms.ao->SetStrength(reader->GetFloat("AOStrength"));
	parms.ao->SetRadius(reader->GetFloat("AORadius"));
	parms.ao->SetPower(reader->GetFloat("AOPower"));

	Util::String name = reader->GetString("Id");
	parms.common->SetName(name);
	float blend = reader->GetOptFloat("PEBlendTime", 0.0f);
	parms.common->SetBlendTime(blend);

	fxStream->Close();
	reader->Close();
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
PostEffectParser::Save(const Util::String & path, const PostEffect::PostEffectEntity::ParamSet & parms)
{
	Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(path);
	stream->SetAccessMode(IO::Stream::WriteAccess);
	Ptr<IO::XmlWriter> writer = IO::XmlWriter::Create();
	writer->SetStream(stream);

	writer->Open();

	writer->BeginNode("PostFX");

	writer->SetMatrix44("GlobalLightTransform", parms.light->GetLightTransform());
	writer->SetFloat4("GlobalLightDiffuse", parms.light->GetLightColor());
	writer->SetFloat4("GlobalLightOpposite", parms.light->GetLightOppositeColor());
	writer->SetFloat4("GlobalLightAmbient", parms.light->GetLightAmbientColor());
	writer->SetBool("GlobalLightCastShadows", parms.light->GetLightCastsShadows());
	writer->SetFloat("GlobalLightShadowIntensity", parms.light->GetLightShadowIntensity());
	writer->SetFloat("GlobalLightIntensity", parms.light->GetLightIntensity());
	writer->SetFloat("GlobalLightBacklightFactor", parms.light->GetBackLightFactor());
	writer->SetFloat("GlobalLightShadowBias", parms.light->GetLightShadowBias());
	writer->SetFloat("Saturation", parms.color->GetColorSaturation());
	writer->SetFloat4("Balance", parms.color->GetColorBalance());
	writer->SetFloat("MaxLuminance", parms.color->GetColorMaxLuminance());
	writer->SetFloat("BloomScale", parms.hdr->GetHdrBloomIntensity());
	writer->SetFloat4("BloomColor", parms.hdr->GetHdrBloomColor());
	writer->SetFloat("BloomThreshold", parms.hdr->GetHdrBloomThreshold());
	writer->SetFloat("FogNearDist", parms.fog->GetFogNearDistance());
	writer->SetFloat("FogFarDist", parms.fog->GetFogFarDistance());
	writer->SetFloat("FogHeight", parms.fog->GetFogHeight());
	writer->SetFloat4("FogColor", parms.fog->GetFogColorAndIntensity());
	writer->SetFloat("FocusDistance", parms.dof->GetFocusDistance());
	writer->SetFloat("FocusLength", parms.dof->GetFocusLength());
	writer->SetFloat("FocusRadius", parms.dof->GetFilterSize());
    writer->SetFloat("SkyContrast", parms.sky->GetSkyContrast());
	writer->SetFloat("SkyRotationFactor", parms.sky->GetSkyRotationFactor());
	writer->SetFloat("SkyBrightness", parms.sky->GetSkyBrightness());
	writer->SetString("SkyTexture", parms.sky->GetSkyTexturePath());
    writer->SetString("ProbeReflectionMap", parms.sky->GetReflectanceTexturePath());
    writer->SetString("ProbeIrradianceMap", parms.sky->GetIrradianceTexturePath());
	writer->SetFloat("AOStrength", parms.ao->GetStrength());
	writer->SetFloat("AORadius", parms.ao->GetRadius());
	writer->SetFloat("AOPower", parms.ao->GetPower());
	writer->SetString("Id", parms.common->GetName());
	writer->SetFloat("PEBlendTime", parms.common->GetBlendTime());

	writer->EndNode();
	writer->Close();
}

}