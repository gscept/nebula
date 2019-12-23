//------------------------------------------------------------------------------
// shaderstatenode.cc
// (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "shaderstatenode.h"
#include "transformnode.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/transformdevice.h"
#include "resources/resourcemanager.h"

#include "shared.h"

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
ShaderStateNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate)
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
		retval = TransformNode::Load(fourcc, tag, reader, immediate);
	}
	return retval;
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderStateNode::Unload()
{
	// free material
	Resources::DiscardResource(this->surRes);

	// destroy table and constant buffer
	CoreGraphics::DestroyResourceTable(this->resourceTable);
	CoreGraphics::DestroyConstantBuffer(this->cbo);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::OnFinishedLoading()
{
	this->sharedShader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:shared.fxb"_atm);
	this->surRes = Resources::CreateResource(this->materialName, this->tag, nullptr, nullptr, true);
	this->materialType = Materials::surfacePool->GetType(this->surRes);
	this->surface = Materials::surfacePool->GetId(this->surRes);
	//this->cbo = CoreGraphics::ShaderCreateConstantBuffer(this->sharedShader, "ObjectBlock");
	this->cbo = CoreGraphics::GetGraphicsConstantBuffer(CoreGraphics::GlobalConstantBufferType::VisibilityThreadConstantBuffer);
	this->cboIndex = CoreGraphics::ShaderGetResourceSlot(this->sharedShader, "ObjectBlock");
	this->resourceTable = CoreGraphics::ShaderCreateResourceTable(this->sharedShader, NEBULA_DYNAMIC_OFFSET_GROUP);
	CoreGraphics::ResourceTableSetConstantBuffer(this->resourceTable, { this->cbo, this->cboIndex, 0, true, false, sizeof(Shared::ObjectBlock), 0 });
	CoreGraphics::ResourceTableCommitChanges(this->resourceTable);

	this->modelVar = CoreGraphics::ShaderGetConstantBinding(this->sharedShader, "Model");
	this->invModelVar = CoreGraphics::ShaderGetConstantBinding(this->sharedShader, "InvModel");
	this->objectIdVar = CoreGraphics::ShaderGetConstantBinding(this->sharedShader, "ObjectId");
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ShaderStateNode::Instance::GetDrawPacketSize() const
{
	// the size of the data field should be multiplied by the amount of resource tables we use
	return sizeof(DrawPacket) + // base size
		sizeof(Materials::SurfaceInstanceId) + // surface instance
		sizeof(SizeT) + // number of tables (only 1, but if more, all subsequent members require a multiplier of the number of tables)
		sizeof(CoreGraphics::ResourceTableId) + // only one table
		this->offsets.Size() * sizeof(uint32) + // offsets
		sizeof(uint32) + // amount of offsets
		sizeof(IndexT) + // only one slot
		sizeof(CoreGraphics::ShaderPipeline) // only one pipeline
		;
}

//------------------------------------------------------------------------------
/**
*/
Models::ModelNode::DrawPacket*
ShaderStateNode::Instance::UpdateDrawPacket(void* mem)
{
	char* buf = (char*) mem;

	// first write header
	Models::ModelNode::DrawPacket* ret = (Models::ModelNode::DrawPacket*)buf;
	buf += sizeof(Models::ModelNode::DrawPacket);

	// setup struct offsets
	ret->surfaceInstance = (Materials::SurfaceInstanceId*)buf;
	buf += sizeof(Materials::SurfaceInstanceId);
	ret->numTables = (SizeT*) buf;
	buf += sizeof(SizeT);
	ret->tables = (CoreGraphics::ResourceTableId*)buf;
	buf += sizeof(CoreGraphics::ResourceTableId);
	ret->offsets = (uint32*) buf;
	buf += sizeof(uint32) * this->offsets.Size();
	ret->numOffsets = (uint32*) buf;
	buf += sizeof(uint32);
	ret->slots = (IndexT*) buf;
	buf += sizeof(IndexT);
	ret->pipelines = (CoreGraphics::ShaderPipeline*) buf;
	buf += sizeof(CoreGraphics::ShaderPipeline);

	// start copying data
	*ret->surfaceInstance = this->surfaceInstance;
	*ret->numTables = 1;

	// this information should all be per table
	IndexT offsetIndex = 0;
	for (IndexT j = 0; j < *ret->numTables; j++)
	{
		ret->tables[j] = this->resourceTable;
	
		// copy offsets
		for (IndexT i = 0; i < this->offsets.Size(); i++)
		{
			ret->offsets[offsetIndex++] = this->offsets[i];
		}

		ret->numOffsets[j] = this->offsets.Size();
		ret->slots[j] = NEBULA_DYNAMIC_OFFSET_GROUP;
		ret->pipelines[j] = CoreGraphics::GraphicsPipeline;
	}

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::Instance::Update()
{
	Shared::ObjectBlock block;
	Math::matrix44::storeu(this->modelTransform, block.Model);
	Math::matrix44::storeu(Math::matrix44::inverse(this->modelTransform), block.InvModel);
	uint offset = CoreGraphics::SetGraphicsConstants(CoreGraphics::GlobalConstantBufferType::VisibilityThreadConstantBuffer, block);
	this->offsets[ObjectTransforms] = offset;
}

} // namespace Models