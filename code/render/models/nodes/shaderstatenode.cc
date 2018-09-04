//------------------------------------------------------------------------------
// shaderstatenode.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "shaderstatenode.h"
#include "transformnode.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/transformdevice.h"
#include "resources/resourcemanager.h"

using namespace Util;
using namespace Math;
namespace Models
{
//------------------------------------------------------------------------------
/**
*/
ShaderStateNode::ShaderStateNode()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ShaderStateNode::~ShaderStateNode()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
ShaderStateNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader)
{
	bool retval = true;
	if (FourCC('MNMT') == fourcc)
	{
		// this isn't used, it's been deprecated, the 'MATE' tag is the relevant one
		Util::String materialName = reader->ReadString();
	}
	else if (FourCC('MATE') == fourcc)
	{
		this->materialName = reader->ReadString() + NEBULAT_SURFACE_EXTENSION;
	}
	else if (FourCC('STXT') == fourcc)
	{
		// ShaderTexture
		StringAtom paramName = reader->ReadString();
		StringAtom paramValue = reader->ReadString();
		String fullTexResId = String(paramValue.AsString() + NEBULAT_TEXTURE_EXTENSION);
	}
	else if (FourCC('SINT') == fourcc)
	{
		// ShaderInt
		StringAtom paramName = reader->ReadString();
		int paramValue = reader->ReadInt();
	}
	else if (FourCC('SFLT') == fourcc)
	{
		// ShaderFloat
		StringAtom paramName = reader->ReadString();
		float paramValue = reader->ReadFloat();
	}
	else if (FourCC('SBOO') == fourcc)
	{
		// ShaderBool
		StringAtom paramName = reader->ReadString();
		bool paramValue = reader->ReadBool();
	}
	else if (FourCC('SFV2') == fourcc)
	{
		// ShaderVector
		StringAtom paramName = reader->ReadString();
		float2 paramValue = reader->ReadFloat2();
	}
	else if (FourCC('SFV4') == fourcc)
	{
		// ShaderVector
		StringAtom paramName = reader->ReadString();
		float4 paramValue = reader->ReadFloat4();
	}
	else if (FourCC('STUS') == fourcc)
	{
		// @todo: implement universal indexed shader parameters!
		// shaderparameter used by multilayered nodes
		int index = reader->ReadInt();
		float4 paramValue = reader->ReadFloat4();
		String paramName("MLPUVStretch");
		paramName.AppendInt(index);
	}
	else if (FourCC('SSPI') == fourcc)
	{
		// @todo: implement universal indexed shader parameters!
		// shaderparameter used by multilayered nodes
		int index = reader->ReadInt();
		float4 paramValue = reader->ReadFloat4();
		String paramName("MLPSpecIntensity");
		paramName.AppendInt(index);
	}
	else
	{
		retval = TransformNode::Load(fourcc, tag, reader);
	}
	return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::OnFinishedLoading()
{
	this->sharedShader = CoreGraphics::ShaderServer::Instance()->GetSharedShader();
	this->material = Resources::CreateResource(this->materialName, "", nullptr, nullptr, false);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::ApplyNodeState()
{
	// apply material
	Materials::MaterialApply(this->material);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::Instance::ApplyNodeInstanceState()
{
	CoreGraphics::TransformDevice* transformDevice = CoreGraphics::TransformDevice::Instance();

	// okay, in cases like this, we would benefit shittons if we could just do one set for the entire struct...
	CoreGraphics::ShaderConstantSet(this->modelVar, this->sharedShader, transformDevice->GetModelTransform());
	CoreGraphics::ShaderConstantSet(this->invModelVar, this->sharedShader, transformDevice->GetInvModelTransform());
	CoreGraphics::ShaderConstantSet(this->modelViewProjVar, this->sharedShader, transformDevice->GetModelViewProjTransform());
	CoreGraphics::ShaderConstantSet(this->modelViewVar, this->sharedShader, transformDevice->GetModelViewTransform());
	CoreGraphics::ShaderConstantSet(this->objectIdVar, this->sharedShader, transformDevice->GetObjectId());
}

} // namespace Models