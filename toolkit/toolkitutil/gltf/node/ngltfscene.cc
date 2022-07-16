//------------------------------------------------------------------------------
//  ngltfscene.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ngltfscene.h"
#include "fbx/character/skinpartitioner.h"
#include "modelutil/modeldatabase.h"
#include "skeletonutil/skeletonbuilder.h"
#include "n3util/n3modeldata.h"
#include <array>

using namespace Util;
using namespace ToolkitUtil;
namespace ToolkitUtil
{
__ImplementSingleton(ToolkitUtil::NglTFScene);
__ImplementClass(ToolkitUtil::NglTFScene, 'ASXS', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
NglTFScene::NglTFScene() :
    isOpen(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
NglTFScene::~NglTFScene()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
NglTFScene::Open()
{
    n_assert(!this->IsOpen());
    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFScene::Close()
{
    n_assert(this->IsOpen());
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFScene::ExtractMeshNodes(const Gltf::Node* node, Math::mat4 parentTransform)
{
    Math::mat4 localTransform;
    if (node->hasTRS)
        localTransform = Math::scaling(node->scale) * Math::rotationquat(node->rotation) * Math::translation(node->translation);
    else
        localTransform = node->matrix;

    Math::mat4 worldTransform = parentTransform * localTransform;

    if (node->mesh != -1)
    {
        Ptr<NglTFMesh> meshNode;
        if (!this->meshNodes.Contains(&this->scene->meshes[node->mesh]))
        {
            // @todo    Gltf supports mesh instancing and we should handle it
            meshNode = NglTFMesh::Create();
            meshNode->Setup(node, this);
            meshNode->ExtractTransform(worldTransform);
            this->meshNodes.Add(&this->scene->meshes[node->mesh], meshNode);
        }
        else
        {
            meshNode = this->meshNodes[&this->scene->meshes[node->mesh]];
        }

        if (!this->nodes.Contains(node))
        {
            this->nodes.Add(node, meshNode);
        }
    }

    for (int child : node->children)
    {
        Gltf::Node* childNode = &this->scene->nodes[child];
        ExtractMeshNodes(childNode, worldTransform);
    }
};

//------------------------------------------------------------------------------
/**
*/
void
NglTFScene::Setup(Gltf::Document const* scene, const ExportFlags& exportFlags, const ExportMode& exportMode, float scale)
{
    n_assert(this->IsOpen());
    n_assert(scene);
    this->scene = scene;

    // create scene mesh
    this->mesh = new MeshBuilder;

    // set export settings
    this->flags = exportFlags;
    this->mode = exportMode;

    // create physics mesh
    this->physicsMesh = new MeshBuilder;

    // set scale
    this->scale = scale;

    Util::Dictionary<int, IndexT> jointNodeToIndex;

    if (!this->scene->skins.IsEmpty())
    {
        for (auto const& skin : this->scene->skins)
        {
            // Build skeleton
            SkeletonBuilder skel;

            // First, create all joint that we know we'll need.
            skel.joints.Resize(skin.joints.Size());

            IndexT index = 0;
            for (auto const jointNode : skin.joints)
            {
                auto const& node = this->scene->nodes[jointNode];
                
                // Create joint map
                jointNodeToIndex.Add(jointNode, index);

                Math::vec3 translation;
                Math::quat rotation;
                Math::vec3 scale;

                if (node.hasTRS)
                {
                    translation = node.translation;
                    rotation = node.rotation;
                    scale = node.scale;
                }
                else
                {
                    decompose(node.matrix, scale, rotation, translation);
                }

                ToolkitUtil::Joint& joint = skel.joints[index];
                joint.index = index;
                joint.name = node.name;
                joint.translation = translation.vec;
                joint.rotation = rotation.vec;
                joint.scale = scale.vec;
                
                index++;
            }

            // update parents for each joint
            for (auto const jointNode : skin.joints)
            {
                auto const& node = this->scene->nodes[jointNode];
                IndexT parent = jointNodeToIndex[jointNode];
                for (auto const child : node.children)
                {
                    IndexT childIndex = jointNodeToIndex[child];
                    skel.joints[childIndex].parent = parent;
                }
            }
        }
    }

    if (!this->scene->animations.IsEmpty())
    {
        AnimBuilder animBuilder;
        for (auto const& animation : this->scene->animations)
        {
            AnimBuilderClip clip;
            clip.SetName(animation.name);
            
            for (auto const& channel : animation.channels)
            {
                AnimBuilderCurve curve;
                Gltf::Animation::Sampler const& sampler = animation.samplers[channel.sampler];
                
                int jointNode = channel.target.node;
                
                //this->scene->accessors[sampler.output].count
            }
        }
    }
    
    // root node
    const int numRootNodes = this->scene->scenes[this->scene->scene].nodes.Size();
    for (int i = 0; i < numRootNodes; i++)
    {
        Gltf::Node* node = &this->scene->nodes[this->scene->scenes[this->scene->scene].nodes[i]];
        // recursively extract meshnodes and calculate their world transforms
        this->ExtractMeshNodes(node, Math::mat4::identity);
    }
}


//------------------------------------------------------------------------------
/**
*/
void
NglTFScene::Cleanup()
{
    IndexT i;
    for (i = 0; i < nodes.Size(); i++)
    {
        this->nodes.ValueAtIndex(i)->Discard();
    }
    delete this->mesh;
    delete this->physicsMesh;
    this->meshNodes.Clear();
    this->nodes.Clear();
}


//------------------------------------------------------------------------------
/**
*/
void
NglTFScene::Flatten()
{
    uint primitiveGroup = 0;
    for (auto const& kvp : this->meshNodes)
    {
        Ptr<NglTFMesh> meshNode = kvp.Value();
        
        // We need to remap the primitive groups
        Util::Array<MeshPrimitive>& primitives = meshNode->Primitives();

        MeshBuilder* rootMesh = this->mesh;
        MeshBuilder* sourceMesh = meshNode->meshBuilder;

        SizeT vertOffset = rootMesh->GetNumVertices();

        SizeT vertCount = sourceMesh->GetNumVertices();
        IndexT vertIndex;
        SizeT triCount = sourceMesh->GetNumTriangles();
        IndexT triIndex;

        for (vertIndex = 0; vertIndex < sourceMesh->GetNumVertices(); vertIndex++)
        {
            rootMesh->AddVertex(sourceMesh->VertexAt(vertIndex));
        }

        // Recalculate group id
        IndexT groupIndex;
        for (groupIndex = 0; groupIndex < primitives.Size(); groupIndex++)
        {
            auto& group = primitives[groupIndex];

            const IndexT first = group.group.GetFirstTriangleIndex();
            const SizeT end = group.group.GetFirstTriangleIndex() + group.group.GetNumTriangles();
            for (triIndex = first; triIndex < end; triIndex++)
            {
                // take old triangle
                MeshBuilderTriangle& tri = sourceMesh->TriangleAt(triIndex);

                // update it
                tri.SetGroupId(primitiveGroup);
            }

            group.group.SetGroupId(primitiveGroup);
            group.group.SetFirstTriangleIndex(first + rootMesh->GetNumTriangles());
            primitiveGroup++;
        }
        
        // then add triangles and update vertex indices
        for (triIndex = 0; triIndex < triCount; triIndex++)
        {
            MeshBuilderTriangle tri = sourceMesh->TriangleAt(triIndex);
            tri.SetVertexIndex(0, vertOffset + tri.GetVertexIndex(0));
            tri.SetVertexIndex(1, vertOffset + tri.GetVertexIndex(1));
            tri.SetVertexIndex(2, vertOffset + tri.GetVertexIndex(2));

            // add triangle to mesh
            rootMesh->AddTriangle(tri);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Searches all lists to find key, emits en error if the node wasn't found (use HasNode to ensure existence)
*/
NglTFNode*
NglTFScene::GetNode(Gltf::Node const* key)
{
    IndexT index = this->nodes.FindIndex(key);
    if (index != InvalidIndex)
    {
        return this->nodes.ValueAtIndex(index);
    }
    else
    {
        n_error("Node with name: '%s' is not registered!", key->name.AsCharPtr());
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
NglTFScene::HasNode(Gltf::Node const* key)
{
    return this->nodes.FindIndex(key) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
NglTFMesh*
NglTFScene::GetMeshNode(Gltf::Mesh const* key)
{
    n_assert(this->meshNodes.Contains(key));
    return this->meshNodes[key];
}

//------------------------------------------------------------------------------
/**
*/
bool
NglTFScene::HasMeshNode(Gltf::Mesh const* key)
{
    return this->meshNodes.Contains(key);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Ptr<NglTFMesh> >
NglTFScene::GetMeshNodes() const
{
    return this->meshNodes.ValuesAsArray();
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFScene::RemoveMeshNode(const Ptr<NglTFMesh>& node)
{
    this->meshNodes.Erase(node->gltfMesh);
}

//------------------------------------------------------------------------------
/**
    Converts import mode to string.
    Each value separated by a pipe means the mesh has this specific feature, and is only compliant with materials with the same features.
*/
const Util::String
NglTFScene::GetSceneFeatureString()
{
    Util::String featureString;
    switch (this->mode)
    {
    case Static:
        featureString = "static";
        break;
    case Skeletal:
        featureString = "skeletal";
        break;
    default:
        featureString = "static";
    }

    if (this->flags & ToolkitUtil::ImportColors) featureString += "|vertexcolors";
    if (this->flags & ToolkitUtil::ImportSecondaryUVs) featureString += "|secondaryuvs";
    return featureString;
}

}
