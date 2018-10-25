#pragma once
//------------------------------------------------------------------------------
/**
	The shader state node wraps the shader associated with a certain primitive node,
	or group of primitive nodes.
	
	(C)2017-2018 Individual contributors, see AUTHORS file
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
		CoreGraphics::ConstantBufferId cbo;
		CoreGraphics::ResourceTableId resourceTable;
		CoreGraphics::ConstantBinding modelVar;
		CoreGraphics::ConstantBinding invModelVar;
		CoreGraphics::ConstantBinding modelViewProjVar;
		CoreGraphics::ConstantBinding modelViewVar;
		CoreGraphics::ConstantBinding objectIdVar;

		uint32 instance;
		Materials::SurfaceInstanceId surfaceInstance;
		Util::FixedArray<uint32> offsets;

		void ApplyNodeInstanceState() override;
		void Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent) override;

		const Materials::SurfaceInstanceId GetSurfaceInstance() const { return this->surfaceInstance; };
	};

	/// create instance
	virtual ModelNode::Instance* CreateInstance(byte** memory, const Models::ModelNode::Instance* parent) override;
	/// get size of instance
	virtual const SizeT GetInstanceSize() const { return sizeof(Instance); }

	/// get surface
	const Materials::SurfaceId GetSurface() const { return this->surface; };
	/// apply node-level state
	virtual void ApplyNodeState();

protected:
	friend class StreamModelPool;

	friend void Visibility::VisibilitySortJob(const Jobs::JobFuncContext& ctx);

	/// load shader state
	bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);
	/// called when loading finished
	virtual void OnFinishedLoading();
	
	Materials::MaterialType* materialType;
	Materials::SurfaceId surface;
	Materials::SurfaceResourceId surRes;
	Resources::ResourceName materialName;

	CoreGraphics::ShaderId sharedShader;
	CoreGraphics::ConstantBufferId cbo;
	IndexT cboIndex;
	CoreGraphics::ResourceTableId resourceTable;
	CoreGraphics::ConstantBinding modelVar;
	CoreGraphics::ConstantBinding invModelVar;
	CoreGraphics::ConstantBinding modelViewProjVar;
	CoreGraphics::ConstantBinding modelViewVar;
	CoreGraphics::ConstantBinding objectIdVar;
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
	CoreGraphics::ConstantBufferId cbo = sparent->cbo;
	this->cbo = cbo;
	this->resourceTable = sparent->resourceTable;
	this->offsets.Resize(2);
	bool rebind = CoreGraphics::ConstantBufferAllocateInstance(cbo, this->offsets[0], this->instance);
	this->offsets[1] = 0; // instancing offset
	if (rebind)
	{
		CoreGraphics::ResourceTableSetConstantBuffer(sparent->resourceTable, { sparent->cbo, sparent->cboIndex, 0, true, false, -1, 0 });
		CoreGraphics::ResourceTableCommitChanges(sparent->resourceTable);
	}
	this->modelVar = sparent->modelVar;
	this->invModelVar = sparent->invModelVar;
	this->modelViewProjVar = sparent->modelViewProjVar;
	this->modelViewVar = sparent->modelViewVar;
	this->objectIdVar = sparent->objectIdVar;

	// create surface instance
	this->surfaceInstance = sparent->materialType->CreateSurfaceInstance(sparent->surface);
}

} // namespace Models