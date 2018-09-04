#pragma once
//------------------------------------------------------------------------------
/**
	The shader state node wraps the shader associated with a certain primitive node,
	or group of primitive nodes.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "resources/resourceid.h"
#include "coregraphics/shader.h"
#include "materials/materialserver.h"
#include "transformnode.h"
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
		CoreGraphics::ShaderStateId sharedShader;
		CoreGraphics::ShaderConstantId modelVar;
		CoreGraphics::ShaderConstantId invModelVar;
		CoreGraphics::ShaderConstantId modelViewProjVar;
		CoreGraphics::ShaderConstantId modelViewVar;
		CoreGraphics::ShaderConstantId objectIdVar;
		IndexT bufferIndex;

		void ApplyNodeInstanceState() override;
		void Setup(const Models::ModelNode* parent) override;
	};

	/// create instance
	virtual ModelNode::Instance* CreateInstance(Memory::ChunkAllocator<MODEL_INSTANCE_MEMORY_CHUNK_SIZE>& alloc) const;
	/// apply node-level state
	virtual void ApplyNodeState();

protected:
	friend class StreamModelPool;

	/// load shader state
	bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);
	/// called when loading finished
	virtual void OnFinishedLoading();

	CoreGraphics::ShaderId sharedShader;
	Resources::ResourceId material;
	Resources::ResourceName materialName;
};

ModelNodeInstanceCreator(ShaderStateNode)

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderStateNode::Instance::Setup(const Models::ModelNode* parent)
{
	TransformNode::Instance::Setup(parent);
	this->sharedShader = CoreGraphics::ShaderCreateState(static_cast<const ShaderStateNode*>(parent)->sharedShader, { NEBULAT_DYNAMIC_OFFSET_GROUP }, false);
	this->modelVar = CoreGraphics::ShaderStateGetConstant(this->sharedShader, "Model");
	this->invModelVar = CoreGraphics::ShaderStateGetConstant(this->sharedShader, "InvModel");
	this->modelViewProjVar = CoreGraphics::ShaderStateGetConstant(this->sharedShader, "ModelViewProjection");
	this->modelViewVar = CoreGraphics::ShaderStateGetConstant(this->sharedShader, "ModelView");
	this->objectIdVar = CoreGraphics::ShaderStateGetConstant(this->sharedShader, "ObjectId");
	this->type = ShaderStateNodeType;
}

} // namespace Models