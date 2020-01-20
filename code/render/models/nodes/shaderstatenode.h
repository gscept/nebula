#pragma once
//------------------------------------------------------------------------------
/**
	The shader state node wraps the shader associated with a certain primitive node,
	or group of primitive nodes.
	
	(C)2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "resources/resourceid.h"
#include "coregraphics/shader.h"
#include "materials/materialserver.h"
#include "transformnode.h"
#include "jobs/jobs.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/resourcetable.h"

namespace Visibility
{
void VisibilitySortJob(const Jobs::JobFuncContext& ctx);
}

namespace Models
{
class ShaderStateNode : public TransformNode
{
public:
	/// constructor
	ShaderStateNode();
	/// destructor
	virtual ~ShaderStateNode();

	struct Instance : public TransformNode::Instance
	{
		enum DynamicOffsetType
		{
			ObjectTransforms,			// always included from shared.fxh
			InstancingTransforms,		// always included from shared.fxh
			Skinning,					// always included from shared.fxh
			Optional
		};
		CoreGraphics::ResourceTableId resourceTable;

		uint32 instance;
		Materials::SurfaceInstanceId surfaceInstance;
		Util::FixedArray<uint32> offsets;

		IndexT objectTransformsIndex;
		IndexT instancingTransformsIndex;
		IndexT skinningTransformsIndex;

		static const uint NumTables = 1;

		/// setup instance
		void Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent) override;

		/// return size of draw packet for allocation
		virtual SizeT GetDrawPacketSize() const override;
		/// fill draw packet
		virtual Models::ModelNode::DrawPacket* UpdateDrawPacket(void* mem) override;

		/// update prior to drawing
		void Update() override;

		/// another draw function
		void Draw(const SizeT numInstances, Models::ModelNode::DrawPacket* packet);
	};

	/// create instance
	virtual ModelNode::Instance* CreateInstance(byte** memory, const Models::ModelNode::Instance* parent) override;
	/// get size of instance
	virtual const SizeT GetInstanceSize() const { return sizeof(Instance); }

	/// get surface
	const Materials::SurfaceId GetSurface() const { return this->surface; };

protected:
	friend class StreamModelPool;

	friend void Visibility::VisibilitySortJob(const Jobs::JobFuncContext& ctx);

	/// load shader state
	bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate) override;
	/// unload data
	virtual void Unload() override;
	/// called when loading finished
	virtual void OnFinishedLoading();
	
	Materials::MaterialType* materialType;
	Materials::SurfaceId surface;
	Materials::SurfaceResourceId surRes;
	Resources::ResourceName materialName;

	IndexT objectTransformsIndex;
	IndexT instancingTransformsIndex;
	IndexT skinningTransformsIndex;

	CoreGraphics::ResourceTableId resourceTable;
};

ModelNodeInstanceCreator(ShaderStateNode)

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderStateNode::Instance::Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent)
{
	TransformNode::Instance::Setup(node, parent);
	ShaderStateNode* sparent = static_cast<ShaderStateNode*>(node);
	this->resourceTable = sparent->resourceTable;

	this->objectTransformsIndex = sparent->objectTransformsIndex;
	this->instancingTransformsIndex = sparent->instancingTransformsIndex;
	this->skinningTransformsIndex = sparent->skinningTransformsIndex;

	this->offsets.Resize(3); // object data, instance data, skinning data
	this->offsets[this->objectTransformsIndex] = 0;
	this->offsets[this->instancingTransformsIndex] = 0; // instancing offset
	this->offsets[this->skinningTransformsIndex] = 0; // skinning offset

	// create surface instance
	this->surfaceInstance = sparent->materialType->CreateSurfaceInstance(sparent->surface);
}
} // namespace Models