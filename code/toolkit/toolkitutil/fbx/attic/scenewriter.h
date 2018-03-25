#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::SceneWriter
    
    Writes an .n3 scene.
    
    (C) 2012 gscept
*/
#include "core/refcounted.h"
#include "toolkitutil/n3util/n3writer.h"
#include "fbxtypes.h"
#include "util/stack.h"
#include "util/queue.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class SceneWriter : public Core::RefCounted
{
	__DeclareClass(SceneWriter);


public:

	enum WriterMode
	{
		Static,
		Skinned,
		Multilayered,

		NumWriterModes
	};

	enum ShadingMode
	{
		Single,
		Multiple,

		NumShadingModes
	};

	/// constructor
	SceneWriter();
	/// destructor
	virtual ~SceneWriter();

	/// sets the list of meshes
	void SetMeshes(const MeshList& meshes);
	/// sets the write to which the scene should be written
	void SetModelWriter(N3Writer* writer);
	/// sets the writer mode
	void SetWriterMode(WriterMode mode);
	/// sets the shading mode
	void SetShadingMode(ShadingMode mode);
	/// sets the list of pre-defined states
	void SetStates(const Util::Queue<State>& states);
	/// sets the list of pre-defined materials
	void SetMaterials(const Util::Queue<Util::String>& materials);
	/// sets the list of fragments sent from the skin fragmenter (only used for skinned objects)
	void SetFragments(const Util::Dictionary<ShapeNode*, Util::Array<Ptr<SkinFragment> > >& fragments);
	/// sets the file name
	void SetFile(const Util::String& file);
	/// sets the category name
	void SetCategory(const Util::String& category);

	/// write scene
	void WriteScene();
private:

	/// recursively traverses node hierarchy and writes scene
	void WriteNode(ShapeNode* node);

	/// convience function for writing skin fragments
	void WriteFragments(ShapeNode* mesh, 
						const Util::String& name, 
						const ToolkitUtil::Transform& transform, 
						const Math::bbox& boundingBox, 
						int primitiveIndex, 
						const Util::String& skinResource, 
						const ToolkitUtil::State& state, 
						const Util::String& material
						);

	N3Writer* writer;

	MeshList meshes;
	WriterMode writerMode;
	ShadingMode shadingMode;

	Util::String file;
	Util::String category;

	Util::Queue<State> states;
	Util::Queue<Util::String> materials;
	Util::Dictionary<ShapeNode*, Util::Array<Ptr<SkinFragment> > > fragments;

}; 

//------------------------------------------------------------------------------
/**
*/
inline void 
SceneWriter::SetMeshes( const MeshList& meshes )
{
	this->meshes = meshes;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
SceneWriter::SetModelWriter( N3Writer* writer )
{
	n_assert(writer);
	this->writer = writer;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
SceneWriter::SetWriterMode( WriterMode mode )
{
	this->writerMode = mode;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
SceneWriter::SetShadingMode( ShadingMode mode )
{
	this->shadingMode = mode;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
SceneWriter::SetStates( const Util::Queue<State>& states )
{
	this->states = states;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
SceneWriter::SetMaterials( const Util::Queue<Util::String>& materials )
{
	this->materials = materials;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
SceneWriter::SetFragments( const Util::Dictionary<ShapeNode*, Util::Array<Ptr<SkinFragment> > >& fragments )
{
	this->fragments = fragments;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
SceneWriter::SetFile( const Util::String& file )
{
	n_assert(!file.IsEmpty());
	this->file = file;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
SceneWriter::SetCategory( const Util::String& category )
{
	n_assert(!category.IsEmpty());
	this->category = category;
}
} // namespace ToolkitUtil
//------------------------------------------------------------------------------