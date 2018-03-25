#pragma once
//------------------------------------------------------------------------------
/**
    @class Toolkit::N3Writer
    
	Writes a binary model/character/particle
*/
#include "toolkitutil/modelwriter.h"
#include "util/stringatom.h"
#include "util/variant.h"
#include "math/bbox.h"
#include "characters/characterskinlist.h"
#include "n3util/n3writer.h"
#include "toolkitutil/fbx/character/skinfragment.h"
#include "particles/emitterattrs.h"
#include "n3modeldata.h"
#include "physics/model/templates.h"
#include "modelutil/modelconstants.h"


namespace ToolkitUtil
{


class N3Writer : public Core::RefCounted
{
	__DeclareClass(N3Writer);
public:
	N3Writer();
	~N3Writer();

	/// used to write a joint index
	typedef IndexT JointIndex;
	typedef IndexT PrimitiveGroupIndex;

	/// opens the writer using a stream
	void Open(const Ptr<IO::Stream>& stream);
	/// close the writer
	void Close();

	/// returns true if writer is open
	bool IsOpen();

	/// sets the model writer
	void SetModelWriter(Ptr<ModelWriter> writer);

	/// begins a new model/scene
	void BeginModel(const Util::String& modelName);
	/// ends the model/scene
	void EndModel();
	/// begins a root node (note, all model node has to be children of this node!)
	void BeginRoot(const Math::bbox& sceneBox);
	/// ends a root node
	void EndRoot();
	/// begins a skin node
	void BeginSkin(const Util::String& skinName, const Math::bbox& boundingBox);
	/// ends a skin node
	void EndSkin();
	/// begins a character node and writes skin, joints and animation resource
	void BeginCharacter(const Util::String& modelName, const Util::Array<Skinlist>& skins, const Util::Array<Joint>& jointArray, const Util::String& animationResource, const Util::Array<JointMask>& jointMasks);
	/// ends a character node
	void EndCharacter();	
	/// writes a static model using materials
	void BeginStaticModel(const Util::String& name, const Transform& transform, PrimitiveGroupIndex groupIndex, const Math::bbox& boundingBox, const Util::String& meshResource, const State& state, const Util::String& material);	
	/// wrapper function which takes a mesh (skin), a list of shader variables, textures, joints, animations and a material to be written as a character
	void BeginSkinnedModel(const Util::String& name, const Transform& transform, const Math::bbox& boundingBox, int fragmentIndex, int fragmentCount, const Util::Array<ModelConstants::SkinNode>& skinNodes, const Util::String& skinResource, const State& state, const Util::String& material);
	/// wrapper function which takes a mesh, a list of shader variables and textures, to be written as a particle mesh
	void BeginParticleModel(const Util::String& name, const Transform& transform, const Util::String& meshResource, const IndexT primGroup, const State& state, const Util::String& material, const Particles::EmitterAttrs& emitterAttrs);
	/// wrapper function for writing  a physics node
	void BeginPhysicsNode(const Util::String& name);
	/// begins colliders section
	void BeginColliders();
	/// end colliders
	void EndColliders();
	/// writes physics colliders
	void WritePhysicsColliders(const Util::String& name, const Util::Array<Physics::ColliderDescription> & colliders);	
	/// writes physics objects
	void WritePhysicsObjects(const Util::Array<Physics::PhysicsObjectDescription> & objects);
	/// ends physics node
	void EndPhysicsNode();

	/// writes lod info
	void WriteLODDistances(float maxDistance, float minDistance);

	/// wrapper function which ends a model node
	void EndModelNode();

private:
	/// write the core information into model node instance (requires an open model node type)
	void WriteState(const Util::String& meshResource, const Math::bbox& boundingBox, PrimitiveGroupIndex groupIndex, const State& states, const Util::String& material);
	/// write transform node
	void WriteTransform(const Transform& transform);

	int modelCounter;
	bool isBeginRoot;
	bool isBeginCharacter;
	bool isBeginModel;
	bool isOpen;

	Ptr<ModelWriter> modelWriter;
};

} // namespace Toolkit