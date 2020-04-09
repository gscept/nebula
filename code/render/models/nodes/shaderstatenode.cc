//------------------------------------------------------------------------------
// shaderstatenode.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "shaderstatenode.h"
#include "transformnode.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/transformdevice.h"
#include "resources/resourceserver.h"

#include "objects_shared.h"

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
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::OnFinishedLoading()
{
	TransformNode::OnFinishedLoading();
	this->surRes = Resources::CreateResource(this->materialName, this->tag, nullptr, nullptr, true);
	this->materialType = Materials::surfacePool->GetType(this->surRes);
	this->surface = Materials::surfacePool->GetId(this->surRes);
	//this->cbo = CoreGraphics::ShaderCreateConstantBuffer(this->sharedShader, "ObjectBlock");
	CoreGraphics::ShaderId shader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:objects_shared.fxb"_atm);
	CoreGraphics::ConstantBufferId cbo = CoreGraphics::GetGraphicsConstantBuffer(CoreGraphics::GlobalConstantBufferType::VisibilityThreadConstantBuffer);
	this->objectTransformsIndex = CoreGraphics::ShaderGetResourceSlot(shader, "ObjectBlock");
	this->instancingTransformsIndex = CoreGraphics::ShaderGetResourceSlot(shader, "InstancingBlock");
	this->skinningTransformsIndex = CoreGraphics::ShaderGetResourceSlot(shader, "JointBlock");

	this->resourceTable = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_DYNAMIC_OFFSET_GROUP);
	CoreGraphics::ResourceTableSetConstantBuffer(this->resourceTable, { cbo, this->objectTransformsIndex, 0, true, false, sizeof(ObjectsShared::ObjectBlock), 0 });
	CoreGraphics::ResourceTableCommitChanges(this->resourceTable);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ShaderStateNode::Instance::GetDrawPacketSize() const
{
	// the size of the data field should be multiplied by the amount of resource tables we use
	return sizeof(DrawPacket)								// header size
		+ sizeof(Materials::SurfaceInstanceId)				// surface instance
		+ this->offsets.Size() * sizeof(uint32)				// offsets are just one big list, with the numOffsets denoting how many offsets per table should be consumed
		+ sizeof(SizeT)										// number of resource tables
		+ sizeof(CoreGraphics::ResourceTableId) * NumTables // tables
		+ sizeof(uint32) * NumTables						// number of offset lists
		+ sizeof(IndexT) * NumTables						// one slot for resource tables (NEBULA_DYNAMIC_OFFSET_GROUP)
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
	ret->node = this;

	// setup struct offsets
	ret->surfaceInstance = (Materials::SurfaceInstanceId*)buf;
	buf += sizeof(Materials::SurfaceInstanceId);
	ret->offsets = (uint32*)buf;
	buf += sizeof(uint32) * this->offsets.Size();
	ret->numTables = (SizeT*) buf;
	buf += sizeof(SizeT);
	ret->tables = (CoreGraphics::ResourceTableId*)buf;
	buf += sizeof(CoreGraphics::ResourceTableId) * NumTables;
	ret->numOffsets = (uint32*) buf;
	buf += sizeof(uint32) * NumTables;
	ret->slots = (IndexT*) buf;
	buf += sizeof(IndexT) * NumTables;

	*ret->surfaceInstance = this->surfaceInstance;
	*ret->numTables = NumTables;

	ret->offsets = this->offsets.Begin();
	ret->numOffsets[0] = this->offsets.Size();

	ret->slots[0] = NEBULA_DYNAMIC_OFFSET_GROUP;
	ret->tables[0] = this->resourceTable;

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::Instance::Update()
{
	if (!this->dirty)
		return;

	ObjectsShared::ObjectBlock block;
	Math::matrix44::storeu(this->modelTransform, block.Model);
	Math::matrix44::storeu(this->invModelTransform, block.InvModel);
	uint offset = CoreGraphics::SetGraphicsConstants(CoreGraphics::GlobalConstantBufferType::VisibilityThreadConstantBuffer, block);
	this->offsets[this->objectTransformsIndex] = offset;
	this->dirty = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderStateNode::Instance::SetDirty(bool b)
{
	this->dirty = b;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::Instance::Draw(const SizeT numInstances, const IndexT baseInstance, Models::ModelNode::DrawPacket* packet)
{
	CoreGraphics::DrawInstanced(numInstances, baseInstance);
}

} // namespace Models