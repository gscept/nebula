//------------------------------------------------------------------------------
//  modelbuilder.cc
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelbuilder.h"
#include "io/stream.h"
#include "io/ioserver.h"
#include "binarymodelwriter.h"
#include "modelconstants.h"
#include "physics/model/templates.h"
#include "math/transform44.h"
#include "physics/staticobject.h"
#include "physics/physicsbody.h"

using namespace Util;
using namespace IO;
using namespace Particles;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::ModelBuilder, 'MDBU', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
ModelBuilder::ModelBuilder() : 
	constants(0),
	attributes(0),
	physics(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ModelBuilder::~ModelBuilder()
{
    this->constants = 0;
    this->attributes = 0;
    this->physics = 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelBuilder::SaveN3( const IO::URI& uri, Platform::Code platform )
{
	// make sure the target directory exists
	IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

	// create stream
	Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
	stream->SetAccessMode(Stream::WriteAccess);
	if (stream->Open())
	{
		// create binary writer
		Ptr<BinaryModelWriter> binaryWriter = BinaryModelWriter::Create();
		binaryWriter->SetPlatform(platform);

		// create N3 writer
		Ptr<N3Writer> n3Writer = N3Writer::Create();
		n3Writer->SetModelWriter(binaryWriter.upcast<ModelWriter>());

		// open writer
		n3Writer->Open(stream);

		// begin model
		n3Writer->BeginModel(this->constants->GetName());

		if (this->constants->GetCharacterNodes().Size() > 0)
		{
			// write characters
			this->WriteCharacter(n3Writer);
		}
		else
		{
			// begin top-level model node
			n3Writer->BeginRoot(this->constants->GetGlobalBoundingBox());

			// write shapes
			this->WriteShapes(n3Writer);

			// write particles
			this->WriteParticles(n3Writer);

			// write appendix nodes
			this->WriteAppendix(n3Writer);

			// end root
			n3Writer->EndRoot();
		}		

		// end name
		n3Writer->EndModel();

		// close writer
		n3Writer->Close();

		stream->Close();
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelBuilder::SaveN3Physics( const IO::URI& uri, Platform::Code platform )
{

	// make sure the target directory exists
	IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

	// create stream
	Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
	stream->SetAccessMode(Stream::WriteAccess);
	if (stream->Open())
	{
		// create binary writer
		Ptr<BinaryModelWriter> binaryWriter = BinaryModelWriter::Create();
		binaryWriter->SetPlatform(platform);

		// create N3 writer
		Ptr<N3Writer> n3Writer = N3Writer::Create();
		n3Writer->SetModelWriter(binaryWriter.upcast<ModelWriter>());

		// open writer
		n3Writer->Open(stream);

		// begin model
		n3Writer->BeginModel(this->constants->GetName());		

		// write physics
		this->WritePhysics(n3Writer);

		// end name
		n3Writer->EndModel();

		// close writer
		n3Writer->Close();

		stream->Close();
		return true;
	}

	return false;

}
//------------------------------------------------------------------------------
/**
*/
void
ModelBuilder::WriteShapes(const Ptr<N3Writer>& writer)
{
	// get list of shapes
	const Array<ModelConstants::ShapeNode>& shapes = this->constants->GetShapeNodes();

	// iterate over shapes
	IndexT i;
	for (i = 0; i < shapes.Size(); i++)
	{
		// get shape
		const ModelConstants::ShapeNode& shape = shapes[i];

		// get name of shape
		const String& name = shape.name;

		// get state
		const State& state = this->attributes->GetState(shape.path);

		// write shape
		writer->BeginStaticModel(shape.name,
								 shape.transform,
								 shape.primitiveGroupIndex,
								 shape.boundingBox,
								 shape.mesh,
								 state,
								 state.material);

		// write lod if available
		if (shape.useLOD)
		{
			writer->WriteLODDistances(shape.LODMax, shape.LODMin);
		}

		// end model
		writer->EndModelNode();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelBuilder::WritePhysics( const Ptr<N3Writer>& writer )
{
	writer->BeginPhysicsNode("physics");	
	writer->BeginColliders();
	switch(this->physics->GetExportMode())
	{
		case UseBoundingBox:
		case UseBoundingSphere:
		case UseBoundingCapsule:
			{
				Array<Physics::ColliderDescription> colls;
				const Array<ModelConstants::ShapeNode> & nodes = this->constants->GetShapeNodes();
				for(Array<ModelConstants::ShapeNode>::Iterator iter = nodes.Begin();iter != nodes.End();iter++)
				{
					Math::transform44 t;
					t.setposition(iter->transform.position);
					t.setrotate(iter->transform.rotation);
					t.setscale(iter->transform.scale);
					t.setrotatepivot(iter->transform.rotatePivot);
					t.setscalepivot(iter->transform.scalePivot);
					
					Math::matrix44 nodetrans = t.getmatrix();					

					Math::bbox colBox = iter->boundingBox;
					Math::matrix44 newtrans;
					
					colBox.transform(nodetrans);

					nodetrans.translate(Math::vector(iter->boundingBox.center()));

					Physics::ColliderDescription col;
					col.name = iter->name;
					col.transform = nodetrans;
					switch (this->physics->GetExportMode())
					{
					case UseBoundingBox:
						col.type = Physics::ColliderCube;
						col.box.halfWidth = colBox.extents();
						break;
					case UseBoundingSphere:
					{
						col.type = Physics::ColliderSphere;
						Math::vector v = colBox.size();
						col.sphere.radius = 0.5f * Math::n_min(v.x(), Math::n_min(v.y(), v.z()));
					}
					break;
					case UseBoundingCapsule:
					{
						col.type = Physics::ColliderCapsule;
						Math::vector v = colBox.size();
						col.capsule.height = v.y();
						col.capsule.radius = Math::n_min(v.z(), v.x());						
					}
					break;
					default:
						break;
					}
					colls.Append(col);
				}
				const Array<ModelConstants::ParticleNode> & particleNodes = this->constants->GetParticleNodes();
				for(Array<ModelConstants::ParticleNode>::Iterator iter = particleNodes.Begin();iter != particleNodes.End();iter++)
				{					
					
					Math::transform44 t;
					t.setposition(iter->transform.position);
					t.setrotate(iter->transform.rotation);					
					// particles have fairly bogus values, ignore scale if zero
					if(iter->transform.scale.lengthsq()<0.001f)
					{
						t.setscale(Math::vector(1,1,1));
					}
					else
					{
						t.setscale(iter->transform.scale);
					}					
					t.setrotatepivot(iter->transform.rotatePivot);
					t.setscalepivot(iter->transform.scalePivot);

					Math::matrix44 nodetrans = t.getmatrix();					

					Math::bbox colBox = iter->boundingBox;
					Math::matrix44 newtrans;

					colBox.transform(nodetrans);

					nodetrans.translate(Math::vector(iter->boundingBox.center()));

					Physics::ColliderDescription col;
					col.name = iter->name;
					col.transform = nodetrans;
					col.type = Physics::ColliderCube;
					col.box.halfWidth = colBox.extents();
					colls.Append(col);
				}

				const Array<ModelConstants::Skin >& skins = this->constants->GetSkins();
				for (Array<ModelConstants::Skin>::Iterator iter = skins.Begin(); iter != skins.End(); iter++)
				{
					Math::transform44 t;
					t.setposition(iter->transform.position);
					t.setrotate(iter->transform.rotation);
					t.setscale(iter->transform.scale);
					t.setrotatepivot(iter->transform.rotatePivot);
					t.setscalepivot(iter->transform.scalePivot);		


					Math::matrix44 nodetrans = t.getmatrix();					

					Math::bbox colBox = iter->boundingBox;
					Math::matrix44 newtrans;

					colBox.transform(nodetrans);

					nodetrans.translate(Math::vector(iter->boundingBox.center()));

					Physics::ColliderDescription col;
					col.name = iter->name;
					col.transform = nodetrans;
					col.type = Physics::ColliderCube;
					col.box.halfWidth = colBox.extents();
					colls.Append(col);
				}
				writer->WritePhysicsColliders("DefaultCollider",colls);	
			}
			break;
		case UseGraphicsMesh:
			{
				// get list of shapes
				const Array<ModelConstants::ShapeNode>& shapes = this->constants->GetShapeNodes();
				
				for(int i=0;i<shapes.Size();i++)
				{
					Array<Physics::ColliderDescription> colls;								
					Physics::ColliderDescription coll;
					coll.type = Physics::ColliderMesh;
					String temp = shapes[i].mesh;
					temp.SubstituteString("msh", "phymsh");
					coll.mesh.meshResource = temp;
					coll.mesh.primGroup = shapes[i].primitiveGroupIndex;
					coll.mesh.meshType = this->physics->GetMeshMode();
					Math::transform44 t;
					t.setposition(shapes[i].transform.position);
					t.setrotate(shapes[i].transform.rotation);
					t.setscale(shapes[i].transform.scale);
					t.setrotatepivot(shapes[i].transform.rotatePivot);
					t.setscalepivot(shapes[i].transform.scalePivot);
					coll.transform = t.getmatrix();
					colls.Append(coll);
					writer->WritePhysicsColliders(shapes[i].name,colls);					
				}																		
			}
			break;
		case UsePhysics:
			{
				if(this->constants->GetPhysicsNodes().Size()>0)
				{
					// get list of shapes
					const Array<ModelConstants::PhysicsNode>& shapes = this->constants->GetPhysicsNodes();
												
					for(int i=0;i<shapes.Size();i++)
					{
						Array<Physics::ColliderDescription> colls;	
						Physics::ColliderDescription coll;
						coll.type = Physics::ColliderMesh;
						coll.mesh.meshResource = shapes[i].mesh;
						coll.mesh.primGroup = shapes[i].primitiveGroupIndex;
						coll.mesh.meshType = this->physics->GetMeshMode();
						coll.name = shapes[i].name;
						Math::transform44 t;
						t.setposition(shapes[i].transform.position);
						t.setrotate(shapes[i].transform.rotation);
						t.setscale(shapes[i].transform.scale);
						t.setrotatepivot(shapes[i].transform.rotatePivot);
						t.setscalepivot(shapes[i].transform.scalePivot);
						coll.transform = t.getmatrix();
						colls.Append(coll);
						writer->WritePhysicsColliders(shapes[i].name,colls);	
					}																			
				}
				else
				{
					Array<Physics::ColliderDescription> colls;								
					Physics::ColliderDescription coll;
					coll.type = Physics::ColliderMesh;
					coll.mesh.meshType =  this->physics->GetMeshMode();
					coll.mesh.meshResource = this->physics->GetPhysicsMesh();
					coll.mesh.primGroup = 0;
					colls.Append(coll);
					writer->WritePhysicsColliders(this->physics->GetName(),colls);
				}				
			}
			break;
		default:
			n_error("not implemented");
	}
	writer->EndColliders();
	writer->EndPhysicsNode();
}

//------------------------------------------------------------------------------
/**
*/
void
ModelBuilder::WriteCharacter(const Ptr<N3Writer>& writer)
{
	// get list of characters
	const Array<ModelConstants::CharacterNode>& characters = this->constants->GetCharacterNodes();

	// get character node from constants, but only get the first character
	if (characters.Size() > 0)
	{
		// get character
		const ModelConstants::CharacterNode& character = characters[0];

		// begin character
		writer->BeginCharacter(character.name,
							   character.skinLists,
							   character.joints,
							   character.animation,
							   this->GetAttributes()->GetJointMasks());

		// write skins
		this->WriteSkins(writer);

		// end character
		writer->EndCharacter();

	}
}

//------------------------------------------------------------------------------
/**
*/
void
ModelBuilder::WriteSkins(const Ptr<N3Writer>& writer)
{
	// get list of skins
	const Array<ModelConstants::Skin>& skins = this->constants->GetSkins();

	// get global primitive group counter
	IndexT primGroup = 0;

	// iterate over skins
	IndexT i;
	for (i = 0; i < skins.Size(); i++)
	{
		// get skin
		const ModelConstants::Skin& skin = skins[i];

		// get name of skin
		const String& name = skin.name;

		// get state of name
		const State& state = this->attributes->GetState(skin.path);

		// write skin node
		writer->BeginSkin(skin.name, skin.boundingBox);

		// write skin node
		writer->BeginSkinnedModel(skin.name,
			skin.transform,
			skin.boundingBox,
			primGroup,
			skin.skinFragments.Size(),
			skin.skinFragments,
			skin.mesh,
			state,
			state.material);

		// bump prim group counter
		primGroup += skin.skinFragments.Size();

		// end skin node
		writer->EndModelNode();

		// end skin node
		writer->EndSkin();		
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ModelBuilder::WriteParticles(const Ptr<N3Writer>& writer)
{
	// get list of particles
	const Array<ModelConstants::ParticleNode>& particlesNodes = this->constants->GetParticleNodes();

	// iterate over nodes
	IndexT i;
	for (i = 0; i < particlesNodes.Size(); i++)
	{
		// get particle node
		const ModelConstants::ParticleNode& particleNode = particlesNodes[i];

		// get name of particle
		const String& name = particleNode.name;

		// get state of particle
		const State& state = this->attributes->GetState(particleNode.path);

		// get attributes
		const EmitterAttrs& attrs = this->attributes->GetEmitterAttrs(particleNode.path);

		// get emitter mesh
		const String& emitterMesh = this->attributes->GetEmitterMesh(particleNode.path);

		// begin and close particle (modelnode)
		writer->BeginParticleModel(name, 
										   particleNode.transform,
										   emitterMesh,
										   particleNode.primitiveGroupIndex,
										   state,
										   state.material,
										   attrs);

		writer->EndModelNode();

	}
}

//------------------------------------------------------------------------------
/**
*/
void
ModelBuilder::WriteAppendix(const Ptr<N3Writer>& writer)
{
	const Array<ModelAttributes::AppendixNode>& appendices = this->attributes->GetAppendixNodes();

	IndexT i;
	for (i = 0; i < appendices.Size(); i++)
	{
		// get particle node
		const ModelAttributes::AppendixNode& node = appendices[i];

		// get name of particle
		const String& name = node.name;

		// get state of particle
		const State& state = this->attributes->GetState(node.path);

		if (node.type == ModelAttributes::ParticleNode)
		{
			// get attributes
			const EmitterAttrs& attrs = this->attributes->GetEmitterAttrs(node.path);

			// get emitter mesh
			const String& emitterMesh = this->attributes->GetEmitterMesh(node.path);

			// begin and close particle (modelnode)
			writer->BeginParticleModel(name,
				node.transform,
				emitterMesh,
				node.data.particle.primGroup,
				state,
				state.material,
				attrs);

			writer->EndModelNode();
		}
	}
}

} // namespace ToolkitUtil