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
#include "math/transform.h"
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
                auto body = std::make_unique< PhysicsResource::BodyT>();
                const Array<ModelConstants::ShapeNode> & nodes = this->constants->GetShapeNodes();
                for(Array<ModelConstants::ShapeNode>::Iterator iter = nodes.Begin();iter != nodes.End();iter++)
                {
                    auto newShape = std::make_unique<PhysicsResource::ShapeT>();
                    newShape->collider = std::make_unique<PhysicsResource::ColliderT>();
                    Math::mat4 nodetrans = iter->transform.GetTransform44().getmatrix();

                    Math::bbox colBox = iter->boundingBox;
                    Math::mat4 newtrans;
                    
                    colBox.transform(nodetrans);

                    newShape->material = this->physics->GetMaterial();
                    newShape->transform = Math::transform(colBox.center().vec);

                    switch (this->physics->GetExportMode())
                    {
                    case UseBoundingBox:
                    {
                        PhysicsResource::BoxColliderT col;
                        col.extents = colBox.extents();
                        newShape->collider->data.Set(col);
                        newShape->collider->name = iter->name;
                        newShape->collider->type = Physics::ColliderType_Box;
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
                    body->shapes.push_back(std::move(newShape));
                }
                const Array<ModelConstants::ParticleNode> & particleNodes = this->constants->GetParticleNodes();
                for(Array<ModelConstants::ParticleNode>::Iterator iter = particleNodes.Begin();iter != particleNodes.End();iter++)
                {                   
                    auto newShape = std::make_unique<PhysicsResource::ShapeT>();
                    newShape->collider = std::make_unique<PhysicsResource::ColliderT>();
                    newShape->material = this->physics->GetMaterial();
                    Math::transform44 t = iter->transform.GetTransform44();
                    // particles have fairly bogus values, ignore scale if zero
                    if(lengthsq(t.getscale()) < 0.001f)
                    {
                        t.setscale(Math::vector(1,1,1));
                    }
                    Math::mat4 nodetrans = t.getmatrix();                   

                    Math::bbox colBox = iter->boundingBox;
                    Math::mat4 newtrans;

                    colBox.transform(nodetrans);
                    newShape->transform = Math::transform(iter->transform.position.vec, iter->transform.rotation, iter->transform.scale.vec);

                    PhysicsResource::BoxColliderT col;

                    col.extents = colBox.extents();
                    newShape->collider->data.Set(col);
                    newShape->collider->name = iter->name;
                    newShape->collider->type = Physics::ColliderType_Box;
                    body->shapes.push_back(std::move(newShape));
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
                        newShape->transform = Math::transform(iter->transform.position.vec, iter->transform.rotation, iter->transform.scale.vec);

                        PhysicsResource::BoxColliderT col;

                        col.extents = colBox.extents();
                        newShape->collider->data.Set(col);
                        newShape->collider->name = skinIter.name;
                        newShape->collider->type = Physics::ColliderType_Box;
                        body->shapes.push_back(std::move(newShape));
                    }
                }
                PhysicsResource::BodySetupT* bodySetup = new PhysicsResource::BodySetupT;
                bodySetup->body = std::move(body);
                actor.data.type = PhysicsResource::PhysicsResourceUnion_BodySetup;
                actor.data.value = (void*)bodySetup;
            }
            break;
        case UseGraphicsMesh:
            {
                auto body = std::make_unique< PhysicsResource::BodyT>();
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


                    newShape->transform = Math::transform(shapes[i].transform.position.vec, shapes[i].transform.rotation, shapes[i].transform.scale.vec);
                    newShape->collider->data.Set(newColl);
                    body->shapes.push_back(std::move(newShape));
                }
                const Array<ModelConstants::SkinSetNode>& skinSets = this->constants->GetSkinSetNodes();
                for (Array<ModelConstants::SkinSetNode>::Iterator iter = skinSets.Begin(); iter != skinSets.End(); iter++)
                {
                    Math::mat4 setTransform = iter->transform.GetTransform44().getmatrix();
                    for (ModelConstants::SkinNode const& skinIter : iter->skinFragments)
                    {
                        auto newShape = std::make_unique<PhysicsResource::ShapeT>();
                        newShape->material = this->physics->GetMaterial();
                        newShape->collider = std::make_unique<PhysicsResource::ColliderT>();

                        PhysicsResource::MeshColliderT newColl;
                        newColl.file = skinIter.meshResource;
                        newColl.prim_group = skinIter.primitiveGroupIndex;
                        newColl.type = this->physics->GetMeshMode();
                        newShape->collider->type = Physics::ColliderType_Mesh;
                        newShape->collider->name = skinIter.name;                  
                        
                        newShape->transform = Math::transform(skinIter.transform.position.vec, skinIter.transform.rotation, skinIter.transform.scale.vec);
                        newShape->collider->data.Set(newColl);
                        body->shapes.push_back(std::move(newShape));
                    }
                }
                PhysicsResource::BodySetupT* bodySetup = new PhysicsResource::BodySetupT;
                bodySetup->body = std::move(body);
                actor.data.type = PhysicsResource::PhysicsResourceUnion_BodySetup;
                actor.data.value = (void*)bodySetup;
            }
            break;
        case UsePhysics:
            {
                auto body = std::make_unique< PhysicsResource::BodyT>();
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
                        newShape->transform = Math::transform(shapes[i].transform.position.vec, shapes[i].transform.rotation, shapes[i].transform.scale.vec);
                        newShape->collider->data.Set(newColl);
                        body->shapes.push_back(std::move(newShape));
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
                    body->shapes.push_back(std::move(newShape));
                }         
                PhysicsResource::BodySetupT* bodySetup = new PhysicsResource::BodySetupT;
                bodySetup->body = std::move(body);
                actor.data.type = PhysicsResource::PhysicsResourceUnion_BodySetup;
                actor.data.value = (void*)bodySetup;
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

} // namespace ToolkitUtil