//------------------------------------------------------------------------------
//  modelbuilder.cc
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "modelbuilder.h"
#include "io/stream.h"
#include "io/ioserver.h"
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
ModelBuilder::SaveN3(const IO::URI& uri, Platform::Code platform)
{
    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    // create stream
    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        // create binary writer
        Ptr<BinaryModelWriter> writer = BinaryModelWriter::Create();
        writer->SetPlatform(platform);
        writer->SetStream(stream);

        // open writer
        writer->Open();

        // begin model
        writer->BeginModel("Model", 'MODL', this->constants->GetName());

            writer->BeginModelNode("TransformNode", 'TRFN', "root");

                writer->BeginTag("Scene Bounding Box", 'LBOX');

                const auto& sceneBB = this->constants->GetGlobalBoundingBox();
                writer->WriteVec4(sceneBB.center());
                writer->WriteVec4(sceneBB.extents());

                writer->EndTag();

                // write characters
                this->WriteCharacters(writer);

                // write skins
                this->WriteSkins(writer);

                // write shapes
                this->WriteShapes(writer);

                // write particles
                this->WriteParticles(writer);

            writer->EndModelNode();

        // end name
        writer->EndModel();

        // close writer
        writer->Close();

        stream->Close();
        return true;
    }

    return false;
}
//------------------------------------------------------------------------------
/**
*/
bool
ModelBuilder::SaveN3Physics(const IO::URI& uri, Platform::Code platform)
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
                    t.setrotate(Math::quatyawpitchroll(iter->transform.rotation.y, iter->transform.rotation.x, iter->transform.rotation.z));
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
                    t.setrotate(Math::quatyawpitchroll(iter->transform.rotation.y, iter->transform.rotation.x, iter->transform.rotation.z));
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

                const Array<ModelConstants::SkinSetNode>& skinSets = this->constants->GetSkinSetNodes();
                for (Array<ModelConstants::SkinSetNode>::Iterator iter = skinSets.Begin(); iter != skinSets.End(); iter++)
                {
                    Math::mat4 setTransform = iter->transform.GetTransform44().getmatrix();
                    for (ModelConstants::SkinNode const& skinIter : iter->skinFragments)
                    {
                        auto newShape = std::make_unique<PhysicsResource::ShapeT>();
                        newShape->collider = std::make_unique<PhysicsResource::ColliderT>();
                        newShape->material = this->physics->GetMaterial();
                        Math::transform44 t = iter->transform.GetTransform44();

                        Math::mat4 nodetrans = setTransform * t.getmatrix();
                        Math::bbox colBox = skinIter.boundingBox;

                        colBox.transform(nodetrans);
                        newShape->transform = nodetrans;

                        PhysicsResource::BoxColliderT col;

                        col.extents = colBox.extents();
                        newShape->collider->data.Set(col);
                        newShape->collider->name = skinIter.name;
                        newShape->collider->type = Physics::ColliderType_Cube;
                        actor.shapes.push_back(std::move(newShape));
                    }
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
                    newColl.file = shapes[i].meshResource;
                    newColl.prim_group = shapes[i].primitiveGroupIndex;
                    newColl.type = this->physics->GetMeshMode();
                    newShape->collider->type = Physics::ColliderType_Mesh;
                    newShape->collider->name = shapes[i].name;

                    Math::transform44 t;
                    t.setposition(shapes[i].transform.position);
                    t.setrotate(Math::quatyawpitchroll(shapes[i].transform.rotation.y, shapes[i].transform.rotation.x, shapes[i].transform.rotation.z));
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
                        t.setrotate(Math::quatyawpitchroll(shapes[i].transform.rotation.y, shapes[i].transform.rotation.x, shapes[i].transform.rotation.z));
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
WriteTransform(const Ptr<ModelWriter>& writer, const Transform& transform)
{
    if (transform.position != Math::point())
    {
        writer->BeginTag("Transform Position", 'POSI');
        writer->WriteVec4(transform.position);
        writer->EndTag();
    }

    if (transform.rotation != Math::quat())
    {
        writer->BeginTag("Transform Rotation", 'ROTN');
        writer->WriteVec4(transform.rotation.vec);
        writer->EndTag();
    }

    if (transform.scale != Math::vector())
    {
        writer->BeginTag("Transform Scale", 'SCAL');
        writer->WriteVec4(transform.scale);
        writer->EndTag();
    }

    if (transform.rotatePivot != Math::point())
    {
        writer->BeginTag("Transform Rotation Pivot", 'RPIV');
        writer->WriteVec4(transform.rotatePivot);
        writer->EndTag();
    }

    if (transform.scalePivot != Math::point())
    {
        writer->BeginTag("Transform Scale Pivot", 'SPIV');
        writer->WriteVec4(transform.scalePivot);
        writer->EndTag();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
WriteState(
    const Ptr<ModelWriter>& writer
    , const Util::String& meshResource
    , const IndexT meshIndex
    , const Math::bbox& boundingBox
    , IndexT groupIndex
    , const State& state)
{
    // write mesh
    writer->BeginTag("Mesh Resource", 'MESH');
    writer->WriteString(meshResource);
    writer->EndTag();

    writer->BeginTag("Mesh Index", 'MSHI');
    writer->WriteInt(meshIndex);
    writer->EndTag();

    // write primitive group index
    writer->BeginTag("Primitive Group Index", 'PGRI');
    writer->WriteInt(groupIndex);
    writer->EndTag();

    // write material
    writer->BeginTag("Material", 'MATE');
    writer->WriteString(state.material);
    writer->EndTag();

    // write bounding box
    writer->BeginTag("Bounding Box", 'LBOX');
    writer->WriteVec4(boundingBox.center());
    writer->WriteVec4(boundingBox.extents());
    writer->EndTag();
}

//------------------------------------------------------------------------------
/**
*/
void
ModelBuilder::WriteShapes(const Ptr<ModelWriter>& writer)
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

        // then create actual model with shape node
        writer->BeginModelNode("ShapeNode", 'SPND', name);

            WriteTransform(writer, shape.transform);
            WriteState(writer, shape.meshResource, shape.meshIndex, shape.boundingBox, shape.primitiveGroupIndex, state);

            if (shape.useLOD)
            {
                writer->BeginTag("LODMinDistance", 'SMID');
                writer->WriteFloat(shape.LODMin);
                writer->EndTag();

                writer->BeginTag("LODMaxDistance", 'SMAD');
                writer->WriteFloat(shape.LODMax);
                writer->EndTag();
            }

        writer->EndModelNode();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ModelBuilder::WriteCharacters(const Ptr<ModelWriter>& writer)
{
    // get list of characters
    const Array<ModelConstants::CharacterNode>& characters = this->constants->GetCharacterNodes();

    // get character node from constants, but only get the first character
    if (characters.Size() > 0)
    {
        const auto& jointMasks = this->GetAttributes()->GetJointMasks();
        if (jointMasks.Size() > 0)
        {
            // write number of masks
            writer->BeginTag("Number of masks", 'NJMS');
            writer->WriteInt(jointMasks.Size());
            writer->EndTag();

            // write joint mask
            for (const auto& jointMask : jointMasks)
            {
                writer->BeginTag("Joint mask", 'JOMS');
                writer->WriteString(jointMask.name);
                writer->WriteInt(jointMask.weights.Size());
                for (const auto weight : jointMask.weights)
                {
                    writer->WriteFloat(weight);
                }
                writer->EndTag();
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ModelBuilder::WriteSkins(const Ptr<ModelWriter>& writer)
{
    // get list of skins
    const Array<ModelConstants::SkinSetNode>& skinSets = this->constants->GetSkinSetNodes();

    // iterate over skins
    for (const auto& skinSet : skinSets)
    {
        // get name of skin
        const String& name = skinSet.name;

        writer->BeginModelNode("CharacterSkinNode", 'CHSN', name);
            WriteTransform(writer, skinSet.transform);

            writer->BeginTag("Number of skin fragments", 'NSKF');
            writer->WriteInt(skinSet.skinFragments.Size());
            writer->EndTag();

            for (const auto& skinFragment : skinSet.skinFragments)
            {
                // write the used skin fragments
                writer->BeginTag("Used skin fragments", 'SFRG');
                writer->WriteInt(skinFragment.primitiveGroupIndex);

                // write the used joints for the fragment
                writer->WriteInt(skinFragment.fragmentJoints.Size());

                IndexT j;
                for (j = 0; j < skinFragment.fragmentJoints.Size(); j++)
                {
                    writer->WriteInt(skinFragment.fragmentJoints[j]);
                }
                writer->EndTag();

                // get state of name
                const State& state = this->attributes->GetState(skinFragment.path);

                // write core information such as material, shader variables, textures and mesh. (PGRI is 0 because we have skins)
                WriteState(writer, skinFragment.meshResource, skinFragment.meshIndex, skinFragment.boundingBox, skinFragment.primitiveGroupIndex, state);
            }

        // end skin node
        writer->EndModelNode();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ModelBuilder::WriteParticles(const Ptr<ModelWriter>& writer)
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
        const EmitterAttrs& emitterAttrs = this->attributes->GetEmitterAttrs(particleNode.path);

        // get emitter mesh
        const String& emitterMesh = this->attributes->GetEmitterMesh(particleNode.path);

        writer->BeginModelNode("ParticleSystemNode", 'PSND', name);
            WriteTransform(writer, particleNode.transform);
            WriteState(writer, emitterMesh, particleNode.meshIndex, Math::bbox(), particleNode.primitiveGroupIndex, state);

            // write Emission Frequency Curve
            writer->BeginTag("Emission Frequency", 'EFRQ');
            emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetModFunc());
            writer->EndTag();

            // write Life Time Curve
            writer->BeginTag("Life Time", 'PLFT');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetModFunc());
            writer->EndTag();

            // write Spread Min Curve
            writer->BeginTag("Spread Min", 'PSMN');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetModFunc());
            writer->EndTag();

            // write Spread Max Curve
            writer->BeginTag("Spread Max", 'PSMX');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetModFunc());
            writer->EndTag();

            // write Start Velocity Curve
            writer->BeginTag("Start Velocity", 'PSVL');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetModFunc());
            writer->EndTag();

            // write Rotation Velocity Curve
            writer->BeginTag("Rotation Velocity", 'PRVL');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetModFunc());
            writer->EndTag();

            // write Size Curve
            writer->BeginTag("Size", 'PSZE');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetModFunc());
            writer->EndTag();

            // write Mass Curve
            writer->BeginTag("Mass", 'PMSS');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetModFunc());
            writer->EndTag();

            // write Time Manipulator Curve
            writer->BeginTag("Time Manipulator", 'PTMN');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetModFunc());
            writer->EndTag();

            // write Velocity Factor Curve
            writer->BeginTag("Velocity Factor", 'PVLF');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetModFunc());
            writer->EndTag();

            // write Air Resistance Curve
            writer->BeginTag("Air Resistance", 'PAIR');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetModFunc());
            writer->EndTag();

            // write Red Curve
            writer->BeginTag("Red", 'PRED');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetModFunc());
            writer->EndTag();

            // write Green Curve
            writer->BeginTag("Green", 'PGRN');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetModFunc());
            writer->EndTag();

            // write Blue Curve
            writer->BeginTag("Blue", 'PBLU');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetModFunc());
            writer->EndTag();

            // write Alpha Curve
            writer->BeginTag("Alpha", 'PALP');
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetValues()[0]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetValues()[1]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetValues()[2]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetValues()[3]);
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetKeyPos0());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetKeyPos1());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetFrequency());
            writer->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetAmplitude());
            writer->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetModFunc());
            writer->EndTag();

            // write EmissionDuration
            writer->BeginTag("EmissionDuration", 'PEDU');
            writer->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::EmissionDuration));
            writer->EndTag();

            // write Looping
            writer->BeginTag("Looping", 'PLPE');
            writer->WriteInt(emitterAttrs.GetBool(Particles::EmitterAttrs::Looping));
            writer->EndTag();

            // write Activity Distance
            writer->BeginTag("Activity Distance", 'PACD');
            writer->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::ActivityDistance));
            writer->EndTag();

            // write Render Oldest First
            writer->BeginTag("Render Oldest First", 'PROF');
            writer->WriteInt(emitterAttrs.GetBool(Particles::EmitterAttrs::RenderOldestFirst));
            writer->EndTag();

            // write Billboard
            writer->BeginTag("Billboard", 'PBBO');
            writer->WriteInt(emitterAttrs.GetBool(Particles::EmitterAttrs::Billboard));
            writer->EndTag();

            // write Start Rotation Min
            writer->BeginTag("Start Rotation Min", 'PRMN');
            writer->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::StartRotationMin));
            writer->EndTag();

            // write Start Rotation Max
            writer->BeginTag("Start Rotation Max", 'PRMX');
            writer->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::StartRotationMax));
            writer->EndTag();

            // write Gravity
            writer->BeginTag("Gravity", 'PGRV');
            writer->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::Gravity));
            writer->EndTag();

            // write Particle Stretch
            writer->BeginTag("Particle Stretch", 'PSTC');
            writer->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::ParticleStretch));
            writer->EndTag();

            // write Texture Tile
            writer->BeginTag("Texture Tile", 'PTTX');
            writer->WriteInt((int)(emitterAttrs.GetFloat(Particles::EmitterAttrs::TextureTile)));
            writer->EndTag();

            // write Stretch To Start
            writer->BeginTag("Stretch To Start", 'PSTS');
            writer->WriteInt(emitterAttrs.GetBool(Particles::EmitterAttrs::StretchToStart));
            writer->EndTag();

            // write Velocity Randomize
            writer->BeginTag("Velocity Randomize", 'PVRM');
            writer->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::VelocityRandomize));
            writer->EndTag();

            // write Rotation Randomize
            writer->BeginTag("Rotation Randomize", 'PRRM');
            writer->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::RotationRandomize));
            writer->EndTag();

            // write Size Randomize
            writer->BeginTag("Size Randomize", 'PSRM');
            writer->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::SizeRandomize));
            writer->EndTag();

            // write Precalc Time
            writer->BeginTag("Precalc Time", 'PPCT');
            writer->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::PrecalcTime));
            writer->EndTag();

            // write Randomize Rotation
            writer->BeginTag("Randomize Rotation", 'PRRD');
            writer->WriteInt(emitterAttrs.GetBool(Particles::EmitterAttrs::RandomizeRotation));
            writer->EndTag();

            // write Stretch Detail
            writer->BeginTag("Stretch Detail", 'PSDL');
            writer->WriteInt(emitterAttrs.GetInt(Particles::EmitterAttrs::StretchDetail));
            writer->EndTag();

            // write View Angle Fade
            writer->BeginTag("View Angle Fade", 'PVAF');
            writer->WriteInt(emitterAttrs.GetBool(Particles::EmitterAttrs::ViewAngleFade));
            writer->EndTag();

            // write StartDelay
            writer->BeginTag("StartDelay", 'PDEL');
            writer->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::StartDelay));
            writer->EndTag();

            // write Phases per second
            writer->BeginTag("Phases per second", 'PDPS');
            writer->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::PhasesPerSecond));
            writer->EndTag();

            // write wind direction
            writer->BeginTag("Wind direction", 'WIDR');
            writer->WriteVec4(emitterAttrs.GetVec4(Particles::EmitterAttrs::WindDirection));
            writer->EndTag();

            // write View Angle Fade
            writer->BeginTag("AnimPhases", 'PVAP');
            writer->WriteInt(emitterAttrs.GetInt(Particles::EmitterAttrs::AnimPhases));
            writer->EndTag();

        writer->EndModelNode();

    }
}

} // namespace ToolkitUtil