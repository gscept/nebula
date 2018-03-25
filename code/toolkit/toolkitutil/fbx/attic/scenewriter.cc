//------------------------------------------------------------------------------
//  scenewriter.cc
//  (C) 2011 gscept
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scenewriter.h"

using namespace Util;
using namespace Math;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::SceneWriter, 'SCWR', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
SceneWriter::SceneWriter() : 
	writer(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
SceneWriter::~SceneWriter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
SceneWriter::WriteScene()
{
	// starts a recursion for the root node
	for (int meshIndex = 0; meshIndex < this->meshes.Size(); meshIndex++)
	{
		ShapeNode* mesh = this->meshes[meshIndex];
		this->WriteNode(mesh);

	}
}

//------------------------------------------------------------------------------
/**
*/
void 
SceneWriter::WriteNode( ShapeNode* node )
{

	String name = node->name;
	Transform trans;
	trans.position = node->translation;
	trans.rotation = node->rotation;
	bbox boundingBox = node->boundingBox;
	N3Writer::PrimitiveGroupIndex primitiveIndex = node->primGroup;
	State state;
	String material;
	String meshResource = node->resource;

	if (!this->states.IsEmpty())
	{
		state = this->states.Dequeue();
	}
	else
	{
		state.textures.Append(Texture("DiffuseMap", "tex:system/white"));
		state.textures.Append(Texture("NormalMap", "tex:system/nobump"));
	}

	if (!this->materials.IsEmpty())
	{
		material = this->materials.Dequeue();
	}
	else
	{
		if (this->writerMode == Skinned)
		{
			material = "SolidSkinned";
		}
		else if (this->writerMode == Multilayered)
		{
			material = "SolidMultilayered";
		}
		else if (this->writerMode == Static)
		{
			material = "Solid";
		}
		else
		{
			n_error("Writer mode not suppoted!\n");
		}
	}
	switch (this->writerMode)
	{
	case Static:
		{
			switch (this->shadingMode)
			{
			case Single:
				{
					this->writer->BeginStaticModel(name, trans, primitiveIndex, boundingBox, meshResource, state, "shd:static");
					break;
				}
			case Multiple:
				{
					
					this->writer->BeginStaticMaterialModel(name, trans, primitiveIndex, boundingBox, meshResource, state, material);
					break;
				}
			}
			this->writer->EndModelNode();
			break;
		}
	case Skinned:
		{
			this->writer->BeginSkin(node->name, node->boundingBox);
			this->WriteFragments(node,
								node->name,
								trans,
								boundingBox,
								node->primGroup,
								meshResource,
								state,
								material);
			this->writer->EndSkin();
			break;
		}
	}

	
}

//------------------------------------------------------------------------------
/**
*/
void 
SceneWriter::WriteFragments(ShapeNode* mesh, 
							const Util::String& name, 
							const ToolkitUtil::Transform& transform, 
							const Math::bbox& boundingBox, 
							int primitiveIndex, 
							const Util::String& skinResource, 
							const ToolkitUtil::State& state, 
							const Util::String& material)
{

	// go through every fragment
	Util::Array<Ptr<SkinFragment> > meshFragments = this->fragments[mesh];
	for (int fragIndex = 0; fragIndex < meshFragments.Size(); fragIndex++)
	{
		Ptr<SkinFragment> fragment = meshFragments[fragIndex];
		Util::Array<SkinFragment::JointIndex> fragmentPalette = fragment->GetJointPalette();	
		switch (this->shadingMode)
		{
		case Single:	
			{
				this->writer->BeginSkinnedModel(name + String::FromInt(fragIndex), 
												transform, 
												boundingBox, 
												primitiveIndex, 												
												fragIndex, 
												meshFragments.Size(), 
												fragmentPalette, 
												skinResource, 
												state, 
												"shd:skinned");
				break;
			}
		case Multiple:
			{
				// ignores saved materials, because they might cause a crash if one goes from non-skinned to skinned...
				this->writer->BeginSkinnedMaterialModel(name + String::FromInt(fragIndex), 
														transform, 
														boundingBox, 
														primitiveIndex, 
														fragIndex, 
														meshFragments.Size(), 
														fragmentPalette, 
														skinResource, 
														state, 
														"SolidSkinned");
				break;
			}
		}
		this->writer->EndModelNode();
	}

}


} // namespace ToolkitUtil