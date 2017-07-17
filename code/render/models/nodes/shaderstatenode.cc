//------------------------------------------------------------------------------
// shaderstatenode.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "shaderstatenode.h"
#include "transformnode.h"

using namespace Util;
using namespace Materials;
using namespace Math;
namespace Models
{

__ImplementClass(Models::ShaderStateNode, 'STND', Models::ModelNode);
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
ShaderStateNode::Load(const Util::FourCC& tag, const Ptr<Models::ModelLoader>& loader, const Ptr<IO::BinaryReader>& reader)
{
	bool retval = true;
	if (FourCC('MNMT') == tag)
	{
		// this isn't used, it's been deprecated, the 'MATE' tag is the relevant one
		Util::String materialName = reader->ReadString();
	}
	else if (FourCC('MATE') == tag)
	{
		this->materialName = reader->ReadString();
	}
	else if (FourCC('STXT') == tag)
	{
		// ShaderTexture
		StringAtom paramName = reader->ReadString();
		StringAtom paramValue = reader->ReadString();
		String fullTexResId = String(paramValue.AsString() + NEBULA3_TEXTURE_EXTENSION);
	}
	else if (FourCC('SINT') == tag)
	{
		// ShaderInt
		StringAtom paramName = reader->ReadString();
		int paramValue = reader->ReadInt();
	}
	else if (FourCC('SFLT') == tag)
	{
		// ShaderFloat
		StringAtom paramName = reader->ReadString();
		float paramValue = reader->ReadFloat();
	}
	else if (FourCC('SBOO') == tag)
	{
		// ShaderBool
		StringAtom paramName = reader->ReadString();
		bool paramValue = reader->ReadBool();
	}
	else if (FourCC('SFV2') == tag)
	{
		// ShaderVector
		StringAtom paramName = reader->ReadString();
		float2 paramValue = reader->ReadFloat2();
	}
	else if (FourCC('SFV4') == tag)
	{
		// ShaderVector
		StringAtom paramName = reader->ReadString();
		float4 paramValue = reader->ReadFloat4();
	}
	else if (FourCC('STUS') == tag)
	{
		// @todo: implement universal indexed shader parameters!
		// shaderparameter used by multilayered nodes
		int index = reader->ReadInt();
		float4 paramValue = reader->ReadFloat4();
		String paramName("MLPUVStretch");
		paramName.AppendInt(index);
	}
	else if (FourCC('SSPI') == tag)
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
		retval = TransformNode::Load(fourCC, loader, reader);
	}
	return retval;
}

} // namespace Models