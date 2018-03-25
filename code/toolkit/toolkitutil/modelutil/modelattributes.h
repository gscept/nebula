#pragma once
//------------------------------------------------------------------------------
/**
    @class Importer::ModelAttributes
    
    A model attribute is a per-model settings handler, which handles per-node shader attributes, clips, mesh flags etc.
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "base/exporttypes.h"
#include "n3util/n3modeldata.h"
#include "take.h"
#include "io/stream.h"
#include "particles/emitterattrs.h"

namespace ToolkitUtil
{
class ModelAttributes : public Core::RefCounted
{
	__DeclareClass(ModelAttributes);
public:

	enum AppendixNodeType
	{
		ParticleNode
	};

	struct AppendixNode
	{
		Util::String name;
		Util::String path;
		ToolkitUtil::Transform transform;
		AppendixNodeType type;

		union
		{
			struct ParticleData
			{
				IndexT primGroup;
			} particle;
		} data;
	};
	/// constructor
	ModelAttributes();
	/// destructor
	virtual ~ModelAttributes();

	/// sets the name
	void SetName(const Util::String& name);
	/// gets the name
	const Util::String& GetName() const;

	/// set the scale of the model
	void SetScale(float f);
	/// get the scale of the model
	const float GetScale() const;

	/// sets state for a specific node
	void SetState(const Util::String& node, const ToolkitUtil::State& state);
	/// gets state for a specific node
	const ToolkitUtil::State& GetState(const Util::String& node);
	/// returns true if attributes has a state for the node
	bool HasState(const Util::String& node);
	/// remove state
	void DeleteState(const Util::String& node);

	/// sets emitter attributes for a specific node
	void SetEmitterAttrs(const Util::String& node, const Particles::EmitterAttrs& attrs);
	/// gets emitter attributes for a specific node
	const Particles::EmitterAttrs& GetEmitterAttrs(const Util::String& node);
	/// returns true if emitter attributes exists for node
	bool HasEmitterAttrs(const Util::String& node);
	/// remove emitter attrs for node
	void DeleteEmitterAttrs(const Util::String& node);
	/// sets emitter mesh for a specific node
	void SetEmitterMesh(const Util::String& node, const Util::String& mesh);
	/// gets emitter mesh for a specific node
	const Util::String& GetEmitterMesh(const Util::String& node);	
	/// remove emitter mesh attribute for node
	void DeleteEmitterMesh(const Util::String& node);

	/// adds a take pointer to the list of takes
	void AddTake(const Ptr<Take>& take);
	/// returns take for specific index
	const Ptr<Take>& GetTake(uint index);
	/// returns take with specific name
	const Ptr<Take>& GetTake(const Util::String& name);
	/// returns reference to array of takes
	const Util::Array<Ptr<Take> >& GetTakes() const;
	/// returns true if attributes has the given take
	const bool HasTake(const Ptr<Take>& take);
	/// returns true if attributes has a take with the given name
	const bool HasTake(const Util::String& name);
	/// clears list of takes
	void ClearTakes();

	/// add an appendix node
	void AddAppendixNode(const Util::String& name, const AppendixNode& node);
	/// remove an appendix node
	void DeleteAppendixNode(const Util::String& name);
	/// get appendix node
	const ModelAttributes::AppendixNode& GetAppendixNode(const Util::String& name) const;
	/// get appendix nodes
	const Util::Array<ModelAttributes::AppendixNode> GetAppendixNodes() const;
	/// returns true if appendix node with name is already attached
	const bool HasAppendixNode(const Util::String& name);

	/// set joint mask
	void SetJointMask(const JointMask& mask);
	/// set all joint masks
	void SetJointMasks(const Util::Array<JointMask>& masks);
	/// get all joint masks
	const Util::Array<JointMask>& GetJointMasks() const;

	/// sets the export flags
	void SetExportFlags(const ToolkitUtil::ExportFlags& exportFlags);
	/// gets the export flags
	const ToolkitUtil::ExportFlags& GetExportFlags() const;
	/// sets the export mode
	void SetExportMode(const ToolkitUtil::ExportMode& exportMode);
	/// gets the export mode
	const ToolkitUtil::ExportMode& GetExportMode() const;

	/// clears attributes
	void Clear();

	/// saves attributes to file
	void Save(const Ptr<IO::Stream>& stream);
	/// loads attributes from file
	void Load(const Ptr<IO::Stream>& stream);

private:
	Util::Dictionary<Util::String, AppendixNode> appendixNodes;
	Util::Dictionary<Util::String, ToolkitUtil::State> nodeStateMap;
	Util::Dictionary<Util::String, Particles::EmitterAttrs> particleAttrMap;
	Util::Dictionary<Util::String, Util::String> particleMeshMap;
	Util::String name;
	Util::String checksum;
	float scaleFactor;
	Util::Array<Ptr<Take>> takes;
	Util::Array<JointMask> jointMasks;

	ToolkitUtil::ExportFlags exportFlags;
	ToolkitUtil::ExportMode exportMode;

	static const short Version = 1;
}; 


//------------------------------------------------------------------------------
/**
*/
inline void 
ModelAttributes::SetName( const Util::String& name )
{
	n_assert(name.IsValid());
	this->name = name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String& 
ModelAttributes::GetName() const
{
	return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ModelAttributes::SetScale( float f )
{
	this->scaleFactor = f;
}

//------------------------------------------------------------------------------
/**
*/
inline const float 
ModelAttributes::GetScale() const
{
	return this->scaleFactor;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ModelAttributes::SetExportFlags( const ToolkitUtil::ExportFlags& exportFlags )
{
	this->exportFlags = exportFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline const ToolkitUtil::ExportFlags& 
ModelAttributes::GetExportFlags() const
{
	return this->exportFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ModelAttributes::SetExportMode( const ToolkitUtil::ExportMode& exportMode )
{
	this->exportMode = exportMode;
}

//------------------------------------------------------------------------------
/**
*/
inline const ToolkitUtil::ExportMode& 
ModelAttributes::GetExportMode() const
{
	return this->exportMode;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelAttributes::SetJointMask(const JointMask& mask)
{
	IndexT i = this->jointMasks.FindIndex(mask);
	if (i == InvalidIndex)  this->jointMasks.Append(mask);
	else					this->jointMasks[i].weights = mask.weights;

}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelAttributes::SetJointMasks(const Util::Array<JointMask>& masks)
{
	this->jointMasks = masks;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<JointMask>&
ModelAttributes::GetJointMasks() const
{
	return this->jointMasks;
}

//------------------------------------------------------------------------------
/**
*/
inline const ModelAttributes::AppendixNode&
ModelAttributes::GetAppendixNode(const Util::String& name) const
{
	return this->appendixNodes[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<ModelAttributes::AppendixNode>
ModelAttributes::GetAppendixNodes() const
{
	return this->appendixNodes.ValuesAsArray();
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
ModelAttributes::HasAppendixNode(const Util::String& name)
{
	return this->appendixNodes.Contains(name);
}

} // namespace Importer
//------------------------------------------------------------------------------