//------------------------------------------------------------------------------
//  modelbuilder.cc
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "modelbuilder.h"
#include "io/stream.h"
#include "io/ioserver.h"
#include "binarymodelwriter.h"
#include "io/filestream.h"
#include "modelconstants.h"
#include "math/transform44.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/physics/actor.h"

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
    constants(nullptr),
    attributes(nullptr)
    ,physics(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ModelBuilder::~ModelBuilder()
{
    this->constants = nullptr;
    this->attributes = nullptr;
    this->physics = nullptr;
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

    Util::Blob target = this->WritePhysics();
    if (!target.IsValid())
    {
        return false;
    }

    // create stream
    Ptr<IO::FileStream> stream = IO::IoServer::Instance()->CreateStream(uri).downcast<IO::FileStream>();
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        stream->Write(target.GetPtr(), target.Size());
        stream->Close();
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
/**
*/
Util::Blob
ModelBuilder::WritePhysics()
{
    
    PhysicsResource::ActorT actor;
    switch(this->physics->GetExportMode())
    {
        case UseBoundingBox:
        case UseBoundingSphere:
        case UseBoundingCapsule:
            {
                const Array<ModelConstants::ShapeNode> & nodes = this->constants->GetShapeNodes();
                for(Array<ModelConstants::ShapeNode>::Iterator iter = nodes.Begin();iter != nodes.End();iter++)
                {
                    auto newShape = std::make_unique<PhysicsResource::ShapeT>();
                    newShape->collider = std::make_unique<PhysicsResource::ColliderT>();
                    Math::transform44 t;
                    t.setposition(iter->transform.position);
                    t.setrotate(iter->transform.rotation);
                    t.setscale(iter->transform.scale);
                    t.setrotatepivot(iter->transform.rotatePivot.vec);
                    t.setscalepivot(iter->transform.scalePivot.vec);
                    
                    Math::mat4 nodetrans = t.getmatrix();                   

                    Math::bbox colBox = iter->boundingBox;
                    Math::mat4 newtrans;
                    
                    colBox.transform(nodetrans);

                    newShape->material = this->physics->GetMaterial();
                    newShape->transform = Math::translation(colBox.center().vec);

                    switch (this->physics->GetExportMode())
                    {
                    case UseBoundingBox:
                    {
                        PhysicsResource::BoxColliderT col;
                        col.extents = colBox.extents();
                        newShape->collider->data.Set(col);
                        newShape->collider->name = iter->name;
                        newShape->collider->type = Physics::ColliderType_Cube;
                    }
                    break;
                    case UseBoundingSphere:
                    {
                        PhysicsResource::SphereColliderT col;
                        Math::vector v = colBox.size();
                        col.radius = 0.5f * Math::min(v.x, Math::min(v.y, v.z));
                        newShape->collider->data.Set(col);
                        newShape->collider->name = iter->name;
                        newShape->collider->type = Physics::ColliderType_Sphere;
                    }
                    break;
                    case UseBoundingCapsule:
                    {
                        PhysicsResource::CapsuleColliderT col;
                        Math::vector v = colBox.size();
                        col.halfheight = v.y;
                        col.radius = Math::min(v.z, v.x);
                        newShape->collider->data.Set(col);
                        newShape->collider->name = iter->name;
                        newShape->collider->type = Physics::ColliderType_Capsule;
                    }
                    break;
                    default:
                        break;
                    }
                    actor.shapes.push_back(std::move(newShape));
                }
                const Array<ModelConstants::ParticleNode> & particleNodes = this->constants->GetParticleNodes();
                for(Array<ModelConstants::ParticleNode>::Iterator iter = particleNodes.Begin();iter != particleNodes.End();iter++)
                {                   
                    auto newShape = std::make_unique<PhysicsResource::ShapeT>();
                    newShape->collider = std::make_unique<PhysicsResource::ColliderT>();
                    newShape->material = this->physics->GetMaterial();
                    Math::transform44 t;
                    t.setposition(iter->transform.position);
                    t.setrotate(iter->transform.rotation);                  
                    // particles have fairly bogus values, ignore scale if zero
                    if(lengthsq(iter->transform.scale)<0.001f)
                    {
                        t.setscale(Math::vector(1,1,1));
                    }
                    else
                    {
                        t.setscale(iter->transform.scale);
                    }                   
                    t.setrotatepivot(iter->transform.rotatePivot.vec);
                    t.setscalepivot(iter->transform.scalePivot.vec);

                    Math::mat4 nodetrans = t.getmatrix();                   

                    Math::bbox colBox = iter->boundingBox;
                    Math::mat4 newtrans;

                    colBox.transform(nodetrans);
                    newShape->transform = nodetrans;

                    PhysicsResource::BoxColliderT col;

                    col.extents = colBox.extents();
                    newShape->collider->data.Set(col);
                    newShape->collider->name = iter->name;
                    newShape->collider->type = Physics::ColliderType_Cube;
                    actor.shapes.push_back(std::move(newShape));
                }

                const Array<ModelConstants::Skin >& skins = this->constants->GetSkins();
                for (Array<ModelConstants::Skin>::Iterator iter = skins.Begin(); iter != skins.End(); iter++)
                {
                    auto newShape = std::make_unique<PhysicsResource::ShapeT>();
                    newShape->collider = std::make_unique<PhysicsResource::ColliderT>();
                    newShape->material = this->physics->GetMaterial();
                    Math::transform44 t;
                    t.setposition(iter->transform.position);
                    t.setrotate(iter->transform.rotation);
                    t.setscale(iter->transform.scale);
                    t.setrotatepivot(iter->transform.rotatePivot.vec);
                    t.setscalepivot(iter->transform.scalePivot.vec);

                    Math::mat4 nodetrans = t.getmatrix();                   
                    Math::bbox colBox = iter->boundingBox;
                    
                    colBox.transform(nodetrans);                   
                    newShape->transform = nodetrans;

                    PhysicsResource::BoxColliderT col;

                    col.extents = colBox.extents();
                    newShape->collider->data.Set(col);
                    newShape->collider->name = iter->name;
                    newShape->collider->type = Physics::ColliderType_Cube;
                    actor.shapes.push_back(std::move(newShape));
                }
            }
            break;
        case UseGraphicsMesh:
            {
                // get list of shapes
                const Array<ModelConstants::ShapeNode>& shapes = this->constants->GetShapeNodes();
                
                for(int i=0;i<shapes.Size();i++)
                {
                    auto newShape = std::make_unique<PhysicsResource::ShapeT>();
                    newShape->material = this->physics->GetMaterial();
                    newShape->collider = std::make_unique<PhysicsResource::ColliderT>();
                    
                    PhysicsResource::MeshColliderT newColl;
                    newColl.file = shapes[i].mesh;
                    newColl.prim_group = shapes[i].primitiveGroupIndex;
                    newColl.type = this->physics->GetMeshMode();
                    newShape->collider->type = Physics::ColliderType_Mesh;
                    newShape->collider->name = shapes[i].name;

                    Math::transform44 t;
                    t.setposition(shapes[i].transform.position);
                    t.setrotate(shapes[i].transform.rotation);
                    t.setscale(shapes[i].transform.scale);
                    t.setrotatepivot(shapes[i].transform.rotatePivot.vec);
                    t.setscalepivot(shapes[i].transform.scalePivot.vec);
                    newShape->transform = t.getmatrix();
                    newShape->collider->data.Set(newColl);
                    actor.shapes.push_back(std::move(newShape));
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
                        auto newShape = std::make_unique<PhysicsResource::ShapeT>();
                        newShape->material = this->physics->GetMaterial();
                        newShape->collider = std::make_unique<PhysicsResource::ColliderT>();
                        PhysicsResource::MeshColliderT newColl;
                        newColl.file = shapes[i].mesh;
                        newColl.prim_group = shapes[i].primitiveGroupIndex;
                        newColl.type = this->physics->GetMeshMode();
                        newShape->collider->type = Physics::ColliderType_Mesh;
                        newShape->collider->name = shapes[i].name;

                        Math::transform44 t;
                        t.setposition(shapes[i].transform.position);
                        t.setrotate(shapes[i].transform.rotation);
                        t.setscale(shapes[i].transform.scale);
                        t.setrotatepivot(shapes[i].transform.rotatePivot.vec);
                        t.setscalepivot(shapes[i].transform.scalePivot.vec);
                        newShape->transform = t.getmatrix();
                        newShape->collider->data.Set(newColl);
                        actor.shapes.push_back(std::move(newShape));
                    }                                                                           
                }
                else
                {
                    auto newShape = std::make_unique<PhysicsResource::ShapeT>();
                    newShape->material = this->physics->GetMaterial();
                    newShape->collider = std::make_unique<PhysicsResource::ColliderT>();
                    PhysicsResource::MeshColliderT newColl;
                                        
                    newColl.type = this->physics->GetMeshMode();
                    newColl.file = this->physics->GetPhysicsMesh();
                    newColl.prim_group = 0;
                    newShape->collider->data.Set(newColl);
                    actor.shapes.push_back(std::move(newShape));
                }               
            }
            break;
        default:
            n_error("not implemented");
    }
    return Flat::FlatbufferInterface::SerializeFlatbuffer<PhysicsResource::Actor>(actor);
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
                               character.skeleton,
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