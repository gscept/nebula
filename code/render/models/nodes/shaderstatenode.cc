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
	this->type = ShaderStateNodeType;
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
		this->materialName = reader->ReadString() + NEBULA_SURFACE_EXTENSION;
	}
	else if (FourCC('STXT') == fourcc)
	{
		// ShaderTexture
		StringAtom paramName = reader->ReadString();
		StringAtom paramValue = reader->ReadString();
		String fullTexResId = String(paramValue.AsString() + NEBULA_TEXTURE_EXTENSION);
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
	this->sharedShader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:shared.fxb"_atm);
	this->material = Resources::CreateResource(this->materialName, this->tag, nullptr, nullptr, true);
	this->materialType = Materials::materialPool->GetType(this->material);
	this->cbo = CoreGraphics::ShaderCreateConstantBuffer(this->sharedShader, "ObjectBlock");
	this->cboIndex = CoreGraphics::ShaderGetResourceSlot(this->sharedShader, "ObjectBlock");
	this->resourceTable = CoreGraphics::ShaderCreateResourceTable(this->sharedShader, NEBULA_DYNAMIC_OFFSET_GROUP);
	CoreGraphics::ResourceTableSetConstantBuffer(this->resourceTable, { this->cbo, this->cboIndex, 0, true, false, -1, 0 });
	CoreGraphics::ResourceTableCommitChanges(this->resourceTable);

	this->modelVar = CoreGraphics::ShaderGetConstantBinding(this->sharedShader, "Model");
	this->invModelVar = CoreGraphics::ShaderGetConstantBinding(this->sharedShader, "InvModel");
	this->modelViewProjVar = CoreGraphics::ShaderGetConstantBinding(this->sharedShader, "ModelViewProjection");
	this->modelViewVar = CoreGraphics::ShaderGetConstantBinding(this->sharedShader, "ModelView");
	this->objectIdVar = CoreGraphics::ShaderGetConstantBinding(this->sharedShader, "ObjectId");
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::ApplyNodeState()
{
	// apply material and bind 
	Materials::MaterialApply(this->material);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::Instance::ApplyNodeInstanceState()
{
	TransformNode::Instance::ApplyNodeInstanceState();
	CoreGraphics::TransformDevice* transformDevice = CoreGraphics::TransformDevice::Instance();

	// okay, in cases like this, we would benefit shittons if we could just do one set for the entire struct...
	CoreGraphics::ConstantBufferUpdateInstance(this->cbo, transformDevice->GetModelTransform(), this->instance, this->modelVar);
	CoreGraphics::ConstantBufferUpdateInstance(this->cbo, transformDevice->GetInvModelTransform(), this->instance, this->invModelVar);
	CoreGraphics::ConstantBufferUpdateInstance(this->cbo, transformDevice->GetModelViewProjTransform(), this->instance, this->modelViewProjVar);
	CoreGraphics::ConstantBufferUpdateInstance(this->cbo, transformDevice->GetModelViewTransform(), this->instance, this->modelViewVar);
	CoreGraphics::ConstantBufferUpdateInstance(this->cbo, transformDevice->GetObjectId(), this->instance, this->objectIdVar);

	// apply with offsets
	CoreGraphics::SetResourceTable(this->resourceTable, NEBULA_DYNAMIC_OFFSET_GROUP, CoreGraphics::GraphicsPipeline, this->offsets);
}

} // namespace Models