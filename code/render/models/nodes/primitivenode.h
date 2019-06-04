#pragma once
//------------------------------------------------------------------------------
/**
	A primitive node contains a mesh resource and a primitive group id.

	
	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "math/bbox.h"
#include "coregraphics/primitivegroup.h"
#include "resources/resourceid.h"
#include "shaderstatenode.h"
#include "coregraphics/mesh.h"
namespace Models
{
class PrimitiveNode : public ShaderStateNode
{
public:
	/// constructor
	PrimitiveNode();
	/// destructor
	virtual ~PrimitiveNode();

	struct Instance : public ShaderStateNode::Instance
	{
		/// setup instance
		void Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent) override;

		/// draw instance
		void Draw() override;
	};

	/// create instance
	virtual ModelNode::Instance* CreateInstance(byte** memory, const Models::ModelNode::Instance* parent) override;
	/// get size of instance
	virtual const SizeT GetInstanceSize() const { return sizeof(Instance); }
	/// get the nodes primitive group index
	uint32_t GetPrimitiveGroupIndex() const { return this->primitiveGroupIndex; }
	/// get primitives mesh id
	CoreGraphics::MeshId GetMeshId() const { return this->res; }

protected:
	friend class StreamModelPool;

	/// load primitive
	virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate) override;
	/// unload data
	virtual void Unload() override;
	/// apply state
	void ApplyNodeState() override;

	CoreGraphics::MeshId res;
	uint32_t primitiveGroupIndex;
};

ModelNodeInstanceCreator(PrimitiveNode)

//------------------------------------------------------------------------------
/**
*/
inline void
PrimitiveNode::Instance::Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent)
{
	ShaderStateNode::Instance::Setup(node, parent);
}



} // namespace Models