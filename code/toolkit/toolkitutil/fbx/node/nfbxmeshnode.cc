//------------------------------------------------------------------------------
//  fbxmeshnode.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fbx/node/nfbxmeshnode.h"
#include "meshutil/meshbuildervertex.h"
#include "nfbxscene.h"

namespace ToolkitUtil
{

using namespace Math;
using namespace Util;
using namespace CoreAnimation;
using namespace ToolkitUtil;

__ImplementClass(ToolkitUtil::NFbxMeshNode, 'FBMN', ToolkitUtil::NFbxNode);

//------------------------------------------------------------------------------
/**
*/
NFbxMeshNode::NFbxMeshNode() : 
	skeletonLink(0),
	groupId(0),
	lod(NULL),
	lodIndex(-1),
	meshFlags(NoMeshFlags),
	exportFlags(ToolkitUtil::FlipUVs),
	exportMode(Static)
{
	this->type = NFbxNode::Mesh;
	this->mesh = new MeshBuilder();
}

//------------------------------------------------------------------------------
/**
*/
NFbxMeshNode::~NFbxMeshNode()
{
	delete this->mesh;
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxMeshNode::Setup( FbxNode* node, const Ptr<NFbxScene>& scene )
{
	NFbxNode::Setup(node, scene);
	n_assert(node->GetMesh());
	this->fbxMesh = node->GetMesh();
	this->material = node->GetMaterial(0)->GetName();

	// triangulate mesh if it isn't already...
	if (!this->fbxMesh->IsTriangleMesh())
	{
		n_error("Should already be triangulated");
	}

	// create mask
	uint meshMask = ToolkitUtil::NoMeshFlags;
	if (this->fbxMesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
	{
		meshMask |= ToolkitUtil::HasSkin;
	}

	// tag as physics if parent name is physics, note that this will only work of the direct parent is called physics!
	Ptr<NFbxNode> parent = this->GetParent();
	if (parent.isvalid() && parent->IsPhysics())
	{
		this->material = "physics";
	}

	if (this->fbxMesh->GetUVLayerCount() > 1)
	{
		meshMask |= ToolkitUtil::HasMultipleUVs;
	}

	if (this->fbxMesh->GetElementVertexColorCount() > 0)
	{
		meshMask |= ToolkitUtil::HasVertexColors;
	}

	if (this->fbxMesh->GetElementBinormalCount() > 0)
	{
		meshMask |= ToolkitUtil::HasBinormals;
	}

	if (this->fbxMesh->GetElementTangentCount() > 0)
	{
		meshMask |= ToolkitUtil::HasTangents;
	}

	// get lod group
	if (this->fbxNode->GetParent() != NULL)
	{
		this->lod = this->fbxNode->GetParent()->GetLodGroup();
		if (this->lod != NULL)
		{
			// DO NOT REMOVE THIS. IF YOU DO, YOU DON'T GET THE LODS PROPERLY.
			// ALSO CALLED A FUCKING BUG.
			int numThresholds = this->lod->GetNumThresholds();
			int displayLevels = this->lod->GetNumDisplayLevels();
		}
	}

	// set mask
	this->meshFlags = (ToolkitUtil::MeshFlags)meshMask;
}

//------------------------------------------------------------------------------
/**
*/
void
NFbxMeshNode::Discard()
{
	this->mesh->Clear();
}

//------------------------------------------------------------------------------
/**
	Extracts mesh information from KfbxMesh
*/
void 
NFbxMeshNode::ExtractMesh()
{
	int vertexCount = fbxMesh->GetControlPointsCount();
	int polyCount = fbxMesh->GetPolygonCount();
	int uvCount = fbxMesh->GetUVLayerCount();
	int normalCount = fbxMesh->GetElementNormalCount();
	int binormalCount = fbxMesh->GetElementBinormalCount();
	int tangentCount = fbxMesh->GetElementTangentCount();
	int colorCount = fbxMesh->GetElementVertexColorCount();

    // this is here just to inform if an artist has forgot to apply a UV set or the mesh has no normals prior to importing
    if (vertexCount > 0)
    {
        n_assert2(uvCount > 0, "You need at least one UV-channel or no shader will be applicable!");
        n_assert2(normalCount > 0, "You need at least one set of normals or no shader will be applicable!");
    }    

	// get scale
	float scaleFactor = NFbxScene::Instance()->GetScale() * 1 / float(fbxScene->GetGlobalSettings().GetSystemUnit().GetScaleFactor());

	// reserve mesh
	this->mesh->Reserve(vertexCount, polyCount);

	// setup vertices with basic per-vertex data
	for (int vertex = 0; vertex < vertexCount; vertex++)
	{
		FbxVector4 v = fbxMesh->GetControlPointAt(vertex);
		MeshBuilderVertex meshVertex;
		float4 position = float4::multiply(float4((float)v[0], (float)v[1], (float)v[2], 1.0f), float4(scaleFactor, scaleFactor, scaleFactor, 1));

		meshVertex.SetComponent(MeshBuilderVertex::CoordIndex, position);
		this->mesh->AddVertex(meshVertex);
	}

	// setup triangles
	for (int polygonIndex = 0; polygonIndex < polyCount; polygonIndex++)
	{
		int polygonSize = fbxMesh->GetPolygonSize(polygonIndex);
		n_assert2(polygonSize == 3, "Some polygons seem to not be triangulated, this is not accepted");
		MeshBuilderTriangle meshTriangle;
		meshTriangle.SetGroupId(this->groupId);
		for (int polygonVertexIndex = 0; polygonVertexIndex < polygonSize; polygonVertexIndex++)
		{
			// we want to offset the vertex index with the current size of the mesh
			int polygonVertex = fbxMesh->GetPolygonVertex(polygonIndex, polygonVertexIndex);
			meshTriangle.SetVertexIndex(polygonVertexIndex, polygonVertex);
		}
		this->mesh->AddTriangle(meshTriangle);
	}

	// if we export using a skeletal model, we always need a skin, so if the mesh isn't skinned, we do it manually
	if (this->exportMode == ToolkitUtil::Skeletal)
	{
		if (this->meshFlags & HasSkin)
		{
			// extract skin
			this->ExtractSkin();
		}
		else
		{
			// generate rigid skin if necessary
			this->GenerateRigidSkin();
		}
	}	


	// inflate the mesh to prepare for vertex copying
	this->mesh->Inflate();		

	int vertexId = 0;
	for (int triangleIndex = 0; triangleIndex < polyCount; triangleIndex++)
	{
		MeshBuilderTriangle& triangle = this->mesh->TriangleAt(triangleIndex);
		for (int polygonVertexIndex = 0; polygonVertexIndex < 3; polygonVertexIndex++)
		{
			int polygonVertex = fbxMesh->GetPolygonVertex(triangleIndex, polygonVertexIndex);
			int inflatedIndex = triangle.GetVertexIndex(polygonVertexIndex);
			MeshBuilderVertex& vertexRef = this->mesh->VertexAt(inflatedIndex);

            // extract uvs from the 0th channel
            this->ExtractUVs(polygonVertex, vertexId, 0, vertexRef);

			// extract multilayered uvs, or ordinary uvs if we're not importing as multilayered
			if (this->exportFlags & ToolkitUtil::ImportSecondaryUVs)
			{
                if (this->meshFlags & HasMultipleUVs)
                {
                    // simply extract two uvs
                    this->ExtractUVs(polygonVertex, vertexId, 1, vertexRef);
                }
                else
                {
                    // if mesh has none, flood to 0
                    vertexRef.SetComponent((MeshBuilderVertex::ComponentIndex)(MeshBuilderVertex::Uv1Index), float4(0));
                }
			}

			// extract colors if so requested
			if (this->exportFlags & ToolkitUtil::ImportColors)
			{
				if (this->meshFlags & HasVertexColors)
				{
					// extract colors
					this->ExtractColors(polygonVertex, vertexId, vertexRef);
				}
				else
				{
					// set vertex color to be white if no vertex colors can be extracted from FBX
					vertexRef.SetComponent(MeshBuilderVertex::ColorUB4NIndex, float4(1));
				}
			}

			// extract normals from mesh
			if (!(this->exportFlags & ToolkitUtil::CalcNormals))
			{
				this->ExtractNormals(polygonVertex, vertexId, vertexRef);
			}

			// extract binormals and tangents from mesh
			if (!(this->exportFlags & ToolkitUtil::CalcBinormalsAndTangents))
			{
				if (this->meshFlags & HasBinormals && this->meshFlags & HasTangents)
				{
					this->ExtractBinormalsAndTangents(polygonVertex, vertexId, vertexRef);
				}
			}

			vertexId++;
		}
	}

	// if we want to calculate our own normals, do so...
	if (this->exportFlags & ToolkitUtil::CalcNormals)
	{
		this->CalculateNormals();
	}

	// flip uvs if checked
	if (this->exportFlags & ToolkitUtil::FlipUVs)
	{
		this->mesh->FlipUvs();
	}

	// compute boundingbox
	this->boundingBox = this->mesh->ComputeBoundingBox();

	// calculate binormals and tangents if either the CalcNormals flag is on, or CalcBinormalsAndTangents is on, or if the model contains no binormals or tangents
	if (this->exportFlags & ToolkitUtil::CalcNormals || 
		this->exportFlags & ToolkitUtil::CalcBinormalsAndTangents || 
		!(this->meshFlags & HasBinormals) || 
		!(this->meshFlags & HasTangents))
	{
		// calculates binormals and tangents using normals and the first UV-set
		this->CalculateTangentsAndBinormals();
	}	

	// deflate to remove redundant vertices if flag is set
	if (this->exportFlags & ToolkitUtil::RemoveRedundant)
	{
		// remove redundant vertices
		this->mesh->Cleanup(0);
	}

	// calculate lod index
	if (this->lod != NULL)
	{
		this->lodIndex = this->GetParent()->IndexOfChild(this);
	}

	// clear up FBX mesh
	fbxMesh->Destroy();
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxMeshNode::ExtractUVs( int polygonVertex, int vertexIndex, int uvLayer, MeshBuilderVertex& vertex )
{
	if (uvLayer > 3)
	{
		n_warning("Multilayered texturing only supports up to four layers!");
	}
	FbxGeometryElementUV* uvElement = this->fbxMesh->GetElementUV(uvLayer);
	switch(uvElement->GetMappingMode())
	{
	case FbxGeometryElement::eByPolygonVertex:
		switch (uvElement->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
            {
                FbxVector2 u = uvElement->GetDirectArray().GetAt(vertexIndex);
                vertex.SetComponent((MeshBuilderVertex::ComponentIndex)(MeshBuilderVertex::Uv0Index+uvLayer*2), float4((float)u[0], (float)u[1], 0.0f, 0.0f));
                break;
            }
		case FbxGeometryElement::eIndexToDirect:
			{
                FbxVector2 u = uvElement->GetDirectArray().GetAt(uvElement->GetIndexArray().GetAt(vertexIndex));
				vertex.SetComponent((MeshBuilderVertex::ComponentIndex)(MeshBuilderVertex::Uv0Index+uvLayer*2), float4((float)u[0], (float)u[1], 0.0f, 0.0f));
				break;
			}
		default:
			n_error("UV-coordinates has to be either direct, or indexed to direct");
			break;
		} 
		break;
	case FbxGeometryElement::eByControlPoint:
		switch (uvElement->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			{
				FbxVector2 u = uvElement->GetDirectArray().GetAt(polygonVertex);
				vertex.SetComponent((MeshBuilderVertex::ComponentIndex)(MeshBuilderVertex::Uv0Index+uvLayer*2), float4((float)u[0], (float)u[1], 0.0f, 0.0f));
				break;
			}
		case FbxGeometryElement::eIndexToDirect:
			{
				FbxVector2 u = uvElement->GetDirectArray().GetAt(uvElement->GetIndexArray().GetAt(polygonVertex));
				vertex.SetComponent((MeshBuilderVertex::ComponentIndex)(MeshBuilderVertex::Uv0Index+uvLayer*2), float4((float)u[0], (float)u[1], 0.0f, 0.0f));
				break;
			}
		default:
			n_error("UV-coordinates has to be either direct, or indexed to direct");
			break;
		}
		break;
	default:
		n_error("Mesh has to be made up of control points or vertex polygons!");
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxMeshNode::ExtractNormals( int polygonVertex, int vertexIndex, MeshBuilderVertex& vertex )
{
	FbxGeometryElementNormal* normalElement = this->fbxMesh->GetElementNormal(0);
	switch(normalElement->GetMappingMode())
	{
	case FbxGeometryElement::eByPolygonVertex:
		switch (normalElement->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			{
				FbxVector4 n = normalElement->GetDirectArray().GetAt(vertexIndex);
				if (vertex.HasComponent(MeshBuilderVertex::NormalB4NBit))
				{
					float4 oldNormal = vertex.GetComponent(MeshBuilderVertex::NormalB4NIndex);
					float4 newNormal = float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.5f);
					newNormal = float4::normalize(newNormal);
					vertex.SetComponent(MeshBuilderVertex::NormalB4NIndex, newNormal);
				}
				else
				{
					vertex.SetComponent(MeshBuilderVertex::NormalB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], 0.5f));
				}
				break;
			}
		case FbxGeometryElement::eIndexToDirect:
			{
				FbxVector4 n = normalElement->GetDirectArray().GetAt(normalElement->GetIndexArray().GetAt(vertexIndex));
				if (vertex.HasComponent(MeshBuilderVertex::NormalB4NBit))
				{
					float4 oldNormal = vertex.GetComponent(MeshBuilderVertex::NormalB4NIndex);
					float4 newNormal = float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.5f);
					newNormal = float4::normalize(newNormal);
					vertex.SetComponent(MeshBuilderVertex::NormalB4NIndex, newNormal);
				}
				else
				{
					vertex.SetComponent(MeshBuilderVertex::NormalB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], 0.5f));
				}
				break;
			}
		default:
			n_error("Normals has to be either direct, or indexed to direct");
			break;
		}
		break;
	case FbxGeometryElement::eByControlPoint:
		switch (normalElement->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			{
				FbxVector4 n = normalElement->GetDirectArray().GetAt(polygonVertex);
				if (vertex.HasComponent(MeshBuilderVertex::NormalB4NBit))
				{
					float4 oldNormal = vertex.GetComponent(MeshBuilderVertex::NormalB4NIndex);
					float4 newNormal = float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.5f);
					newNormal = float4::normalize(newNormal);
					vertex.SetComponent(MeshBuilderVertex::NormalB4NIndex, newNormal);
				}
				else
				{
					vertex.SetComponent(MeshBuilderVertex::NormalB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], 0.5f));
				}
				break;
			}
		case FbxGeometryElement::eIndexToDirect:
			{
				FbxVector4 n = normalElement->GetDirectArray().GetAt(normalElement->GetIndexArray().GetAt(polygonVertex));
				if (vertex.HasComponent(MeshBuilderVertex::NormalB4NBit))
				{
					float4 oldNormal = vertex.GetComponent(MeshBuilderVertex::NormalB4NIndex);
					float4 newNormal = float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.5f);
					newNormal = float4::normalize(newNormal);
					vertex.SetComponent(MeshBuilderVertex::NormalB4NIndex, newNormal);
				}
				else
				{
					vertex.SetComponent(MeshBuilderVertex::NormalB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], 0.5f));
				}
				break;
			}
		default:
			n_error("Normals has to be either direct, or indexed to direct");
			break;
		}
		break;
	default:
		n_error("Mesh has to be made up of control points or vertex polygons!");
		break;
	}
}


//------------------------------------------------------------------------------
/**
*/
void
NFbxMeshNode::ExtractBinormalsAndTangents(int polygonVertex, int vertexIndex, ToolkitUtil::MeshBuilderVertex& vertex)
{
	FbxGeometryElementBinormal* binormalElement = this->fbxMesh->GetElementBinormal(0);

	// first load binormals
	switch (binormalElement->GetMappingMode())
	{
	case FbxGeometryElement::eByPolygonVertex:
		switch (binormalElement->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			FbxVector4 n = binormalElement->GetDirectArray().GetAt(vertexIndex);
			if (vertex.HasComponent(MeshBuilderVertex::BinormalB4NBit))
			{
				float4 oldNormal = vertex.GetComponent(MeshBuilderVertex::BinormalB4NIndex);
				float4 newNormal = float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.5f);
				newNormal = float4::normalize(newNormal);
				vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, newNormal);
			}
			else
			{
				vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], 0.5f));
			}
			break;
		}
		case FbxGeometryElement::eIndexToDirect:
		{
			FbxVector4 n = binormalElement->GetDirectArray().GetAt(binormalElement->GetIndexArray().GetAt(vertexIndex));
			if (vertex.HasComponent(MeshBuilderVertex::BinormalB4NBit))
			{
				float4 oldNormal = vertex.GetComponent(MeshBuilderVertex::BinormalB4NIndex);
				float4 newNormal = float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.5f);
				newNormal = float4::normalize(newNormal);
				vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, newNormal);
			}
			else
			{
				vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], 0.5f));
			}
			break;
		}
		default:
			n_error("Normals has to be either direct, or indexed to direct");
			break;
		}
		break;
	case FbxGeometryElement::eByControlPoint:
		switch (binormalElement->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			FbxVector4 n = binormalElement->GetDirectArray().GetAt(polygonVertex);
			if (vertex.HasComponent(MeshBuilderVertex::BinormalB4NBit))
			{
				float4 oldNormal = vertex.GetComponent(MeshBuilderVertex::BinormalB4NIndex);
				float4 newNormal = float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.5f);
				newNormal = float4::normalize(newNormal);
				vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, newNormal);
			}
			else
			{
				vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], 0.5f));
			}
			break;
		}
		case FbxGeometryElement::eIndexToDirect:
		{
			FbxVector4 n = binormalElement->GetDirectArray().GetAt(binormalElement->GetIndexArray().GetAt(polygonVertex));
			if (vertex.HasComponent(MeshBuilderVertex::BinormalB4NBit))
			{
				float4 oldNormal = vertex.GetComponent(MeshBuilderVertex::BinormalB4NIndex);
				float4 newNormal = float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.5f);
				newNormal = float4::normalize(newNormal);
				vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, newNormal);
			}
			else
			{
				vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], 0.5f));
			}
			break;
		}
		default:
			n_error("Normals has to be either direct, or indexed to direct");
			break;
		}
		break;
	default:
		n_error("Mesh has to be made up of control points or vertex polygons!");
		break;
	}

	FbxGeometryElementTangent* tangentElement = this->fbxMesh->GetElementTangent(0);

	// now tangents	
	switch (tangentElement->GetMappingMode())
	{
	case FbxGeometryElement::eByPolygonVertex:
		switch (tangentElement->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			FbxVector4 n = tangentElement->GetDirectArray().GetAt(vertexIndex);
			if (vertex.HasComponent(MeshBuilderVertex::TangentB4NBit))
			{
				float4 oldNormal = vertex.GetComponent(MeshBuilderVertex::TangentB4NIndex);
				float4 newNormal = float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.5f);
				newNormal = float4::normalize(newNormal);
				vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, newNormal);
			}
			else
			{
				vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], 0.5f));
			}
			break;
		}
		case FbxGeometryElement::eIndexToDirect:
		{
			FbxVector4 n = tangentElement->GetDirectArray().GetAt(tangentElement->GetIndexArray().GetAt(vertexIndex));
			if (vertex.HasComponent(MeshBuilderVertex::TangentB4NBit))
			{
				float4 oldNormal = vertex.GetComponent(MeshBuilderVertex::TangentB4NIndex);
				float4 newNormal = float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.5f);
				newNormal = float4::normalize(newNormal);
				vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, newNormal);
			}
			else
			{
				vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], 0.5f));
			}
			break;
		}
		default:
			n_error("Normals has to be either direct, or indexed to direct");
			break;
		}
		break;
	case FbxGeometryElement::eByControlPoint:
		switch (tangentElement->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			FbxVector4 n = tangentElement->GetDirectArray().GetAt(polygonVertex);
			if (vertex.HasComponent(MeshBuilderVertex::TangentB4NBit))
			{
				float4 oldNormal = vertex.GetComponent(MeshBuilderVertex::TangentB4NIndex);
				float4 newNormal = float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.5f);
				newNormal = float4::normalize(newNormal);
				vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, newNormal);
			}
			else
			{
				vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], 0.5f));
			}
			break;
		}
		case FbxGeometryElement::eIndexToDirect:
		{
			FbxVector4 n = tangentElement->GetDirectArray().GetAt(tangentElement->GetIndexArray().GetAt(polygonVertex));
			if (vertex.HasComponent(MeshBuilderVertex::TangentB4NBit))
			{
				float4 oldNormal = vertex.GetComponent(MeshBuilderVertex::TangentB4NIndex);
				float4 newNormal = float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.5f);
				newNormal = float4::normalize(newNormal);
				vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, newNormal);
			}
			else
			{
				vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], 0.5f));
			}
			break;
		}
		default:
			n_error("Normals has to be either direct, or indexed to direct");
			break;
		}
		break;
	default:
		n_error("Mesh has to be made up of control points or vertex polygons!");
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxMeshNode::ExtractColors( int polygonVertex, int vertexIndex, ToolkitUtil::MeshBuilderVertex& vertex )
{
	FbxGeometryElementVertexColor* colorElement = this->fbxMesh->GetElementVertexColor(0);	
	switch(colorElement->GetMappingMode())
	{
	case FbxGeometryElement::eByPolygonVertex:
		switch (colorElement->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			{
				FbxColor n = colorElement->GetDirectArray().GetAt(vertexIndex);
				if (vertex.HasComponent(MeshBuilderVertex::ColorUB4NBit))
				{
					float4 oldColor = vertex.GetComponent(MeshBuilderVertex::ColorUB4NIndex);
					float4 newColor = float4(oldColor.x() + (float)n[0], oldColor.y() + (float)n[1], oldColor.z() + (float)n[2], oldColor.w() + (float)n[3]);
					newColor = float4::normalize(newColor);
					vertex.SetComponent(MeshBuilderVertex::ColorUB4NIndex, newColor);
				}
				else
				{
					vertex.SetComponent(MeshBuilderVertex::ColorUB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], (float)n[3]));
				}
				break;
			}
		case FbxGeometryElement::eIndexToDirect:
			{
				FbxColor n = colorElement->GetDirectArray().GetAt(colorElement->GetIndexArray().GetAt(vertexIndex));
				if (vertex.HasComponent(MeshBuilderVertex::ColorUB4NBit))
				{
					float4 oldColor = vertex.GetComponent(MeshBuilderVertex::ColorUB4NIndex);
					float4 newColor = float4(oldColor.x() + (float)n[0], oldColor.y() + (float)n[1], oldColor.z() + (float)n[2], oldColor.w() + (float)n[3]);
					newColor = float4::normalize(newColor);
					vertex.SetComponent(MeshBuilderVertex::ColorUB4NIndex, newColor);
				}
				else
				{
					vertex.SetComponent(MeshBuilderVertex::ColorUB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], (float)n[3]));
				}
				break;
			}
		default:
			n_error("Normals has to be either direct, or indexed to direct");
			break;
		}
		break;
	case FbxGeometryElement::eByControlPoint:
		switch (colorElement->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
			{
				FbxColor n = colorElement->GetDirectArray().GetAt(polygonVertex);
				if (vertex.HasComponent(MeshBuilderVertex::ColorUB4NBit))
				{
					float4 oldColor = vertex.GetComponent(MeshBuilderVertex::ColorUB4NIndex);
					float4 newColor = float4(oldColor.x() + (float)n[0], oldColor.y() + (float)n[1], oldColor.z() + (float)n[2], oldColor.w() + (float)n[3]);
					newColor = float4::normalize(newColor);
					vertex.SetComponent(MeshBuilderVertex::ColorUB4NIndex, newColor);
				}
				else
				{
					vertex.SetComponent(MeshBuilderVertex::ColorUB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], (float)n[3]));
				}
				break;
			}
		case FbxGeometryElement::eIndexToDirect:
			{
				FbxColor n = colorElement->GetDirectArray().GetAt(colorElement->GetIndexArray().GetAt(polygonVertex));
				if (vertex.HasComponent(MeshBuilderVertex::ColorUB4NBit))
				{
					float4 oldColor = vertex.GetComponent(MeshBuilderVertex::ColorUB4NIndex);
					float4 newColor = float4(oldColor.x() + (float)n[0], oldColor.y() + (float)n[1], oldColor.z() + (float)n[2], oldColor.w() + (float)n[3]);
					newColor = float4::normalize(newColor);
					vertex.SetComponent(MeshBuilderVertex::ColorUB4NIndex, newColor);
				}
				else
				{
					vertex.SetComponent(MeshBuilderVertex::ColorUB4NIndex, float4((float)n[0], (float)n[1], (float)n[2], (float)n[3]));
				}
				break;
			}
		default:
			n_error("Normals has to be either direct, or indexed to direct");
			break;
		}
		break;
	default:
		n_error("Mesh has to be made up of control points or vertex polygons!");
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxMeshNode::CalculateNormals()
{
	int numTriangles = this->mesh->GetNumTriangles();
	for (int triIndex = 0; triIndex < numTriangles; triIndex++)
	{
		MeshBuilderTriangle& tri = this->mesh->TriangleAt(triIndex);
		int vertIndex1, vertIndex2, vertIndex3;
		tri.GetVertexIndices(vertIndex1, vertIndex2, vertIndex3);

		MeshBuilderVertex& v1 = this->mesh->VertexAt(vertIndex1);
		MeshBuilderVertex& v2 = this->mesh->VertexAt(vertIndex2);
		MeshBuilderVertex& v3 = this->mesh->VertexAt(vertIndex3);

		float4 p1 = v1.GetComponent(MeshBuilderVertex::CoordIndex);
		float4 p2 = v2.GetComponent(MeshBuilderVertex::CoordIndex);
		float4 p3 = v3.GetComponent(MeshBuilderVertex::CoordIndex);

		vector normal1 = float4::cross3(p2 - p1, p3 - p1);

		if (v1.HasComponent(MeshBuilderVertex::NormalB4NBit))
		{
			float4 normal = v1.GetComponent(MeshBuilderVertex::NormalB4NIndex) + normal1;
			normal = float4::normalize(normal);
			v1.SetComponent(MeshBuilderVertex::NormalB4NIndex, normal);
		}
		else
		{
			v1.SetComponent(MeshBuilderVertex::NormalB4NIndex, normal1);
		}

		if (v2.HasComponent(MeshBuilderVertex::NormalB4NBit))
		{
			float4 normal = v1.GetComponent(MeshBuilderVertex::NormalB4NIndex) + normal1;
			normal = float4::normalize(normal);
			v2.SetComponent(MeshBuilderVertex::NormalB4NIndex, normal);
		}
		else
		{
			v2.SetComponent(MeshBuilderVertex::NormalB4NIndex, normal1);
		}

		if (v3.HasComponent(MeshBuilderVertex::NormalB4NBit))
		{
			float4 normal = v1.GetComponent(MeshBuilderVertex::NormalB4NIndex) + normal1;
			normal = float4::normalize(normal);
			v3.SetComponent(MeshBuilderVertex::NormalB4NIndex, normal);
		}
		else
		{
			v3.SetComponent(MeshBuilderVertex::NormalB4NIndex, normal1);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxMeshNode::CalculateTangentsAndBinormals()
{
	int numVertices = mesh->GetNumVertices();
	int numTriangles = mesh->GetNumTriangles();
	float4* tangents1 = new float4[numVertices * 2];
	float4* tangents2 = tangents1 + numVertices;

	memset(tangents1, 0, numVertices * sizeof(float4) * 2);
	for (int triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
	{
		MeshBuilderTriangle& triangle = mesh->TriangleAt(triangleIndex);
		int v1Index = triangle.GetVertexIndex(0);
		int v2Index = triangle.GetVertexIndex(1);
		int v3Index = triangle.GetVertexIndex(2);

		MeshBuilderVertex& vertex1 = mesh->VertexAt(v1Index);
		MeshBuilderVertex& vertex2 = mesh->VertexAt(v2Index);
		MeshBuilderVertex& vertex3 = mesh->VertexAt(v3Index);

		// v1 normal
		const float4& v1 = vertex1.GetComponent(MeshBuilderVertex::CoordIndex);
		// v2 normal
		const float4& v2 = vertex2.GetComponent(MeshBuilderVertex::CoordIndex);
		// v3 normal
		const float4& v3 = vertex3.GetComponent(MeshBuilderVertex::CoordIndex);

		// v1 texture coordinate
		const float4& w1 = vertex1.GetComponent(MeshBuilderVertex::Uv0Index);
		// v2 texture coordinate
		const float4& w2 = vertex2.GetComponent(MeshBuilderVertex::Uv0Index);
		// v3 texture coordinate
		const float4& w3 = vertex3.GetComponent(MeshBuilderVertex::Uv0Index);

		float x1 = v2.x() - v1.x();
		float x2 = v3.x() - v1.x();
		float y1 = v2.y() - v1.y();
		float y2 = v3.y() - v1.y();
		float z1 = v2.z() - v1.z();
		float z2 = v3.z() - v1.z();

		float s1 = w2.x() - w1.x();
		float s2 = w3.x() - w1.x();
		float t1 = w2.y() - w1.y();
		float t2 = w3.y() - w1.y();

		float rDenom = (s1 * t2 - s2 * t1);
		float r = 1/rDenom;

		float4 sdir = float4((t2 * x1 - t1*x2) * r, (t2*y1 - t1*y2) * r, (t2*z1 - t1*z2) * r, 0.0f);
		float4 tdir = float4((s1 * x2 - s2*x1) * r, (s1*y2 - s2*y1) * r, (s1*z2 - s2*z1) * r, 0.0f);

		tangents1[v1Index] += sdir;
		tangents1[v2Index] += sdir;
		tangents1[v3Index] += sdir;

		tangents2[v1Index] += tdir;
		tangents2[v2Index] += tdir;
		tangents2[v3Index] += tdir;
	}

	for (int vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
	{
		MeshBuilderVertex& vertex = mesh->VertexAt(vertexIndex);
		float4 n = float4::normalize(vertex.GetComponent(MeshBuilderVertex::NormalB4NIndex));
		float4 t = tangents1[vertexIndex];
		float4 b = tangents2[vertexIndex];

		float4 tangent = float4::normalize(t - n * float4::dot3(n, t));
		float handedNess = (float4::dot3(float4::cross3(n, t), tangents2[vertexIndex]) < 0.0f ? 1.0f : -1.0f);
		float4 bitangent = float4::normalize(float4::cross3(n, tangent) * handedNess);

		if (vertex.HasComponent(MeshBuilderVertex::TangentB4NBit))
		{
			float4 oldTangent = vertex.GetComponent(MeshBuilderVertex::TangentB4NIndex);
			float4 newTangent = oldTangent + tangent;
			vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, newTangent);
		}
		else
		{
			vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, tangent);
		}

		if (vertex.HasComponent(MeshBuilderVertex::BinormalB4NBit))
		{
			float4 oldBinormal = vertex.GetComponent(MeshBuilderVertex::BinormalB4NIndex);
			float4 newBinormal = oldBinormal + bitangent;
			vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, newBinormal);
		}
		else
		{
			vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, bitangent);
		}
	}

	// finally normalize all of it!
	for (int vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
	{
		MeshBuilderVertex& vertex = mesh->VertexAt(vertexIndex);
		vertex.SetComponent(MeshBuilderVertex::NormalB4NIndex, float4::normalize(vertex.GetComponent(MeshBuilderVertex::NormalB4NIndex)));
		vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, float4::normalize(vertex.GetComponent(MeshBuilderVertex::TangentB4NIndex)));
		vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, float4::normalize(vertex.GetComponent(MeshBuilderVertex::BinormalB4NIndex)));
	}

	delete [] tangents1;
}

//------------------------------------------------------------------------------
/**
	Hmm, maybe a way to check if we have more than 255 indices, then we should switch to using Joint indices as complete uints?
*/
void 
NFbxMeshNode::ExtractSkin()
{
	int vertexCount = this->fbxMesh->GetControlPointsCount();
	int skinCount = this->fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
	int* jointArray = new int[vertexCount*4];
	double* weightArray = new double[vertexCount*4];
	int* slotArray = new int[vertexCount];
	memset(jointArray, 0, sizeof(int)*vertexCount*4);
	memset(weightArray, 0, sizeof(double)*vertexCount*4);
	memset(slotArray, 0, sizeof(int)*vertexCount);
	int maxIndex = 0;

	for (int skinIndex = 0; skinIndex < skinCount; skinIndex++)
	{
		FbxSkin* skin = static_cast<FbxSkin*>(this->fbxMesh->GetDeformer(skinIndex, FbxDeformer::eSkin));
		int clusterCount = skin->GetClusterCount();

		for (int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
		{
			FbxCluster* cluster = skin->GetCluster(clusterIndex);
			FbxNode* joint = cluster->GetLink();
			Ptr<NFbxNode> node = NFbxScene::Instance()->GetNode(joint);
			if (node->IsA(NFbxJointNode::RTTI))
			{
				// get joint node
				Ptr<NFbxJointNode> jointNode = node.downcast<NFbxJointNode>();

				int clusterVertexIndexCount = cluster->GetControlPointIndicesCount();
				for (int vertexIndex = 0; vertexIndex < clusterVertexIndexCount; vertexIndex++)
				{
					int vertex = cluster->GetControlPointIndices()[vertexIndex];
					double weight = cluster->GetControlPointWeights()[vertexIndex];
					int stride = slotArray[vertex];

					// this is just a fail safe for smooth operators
					if (vertex >= vertexCount)
						continue;

					int* jointData = jointArray + (vertex*4);
					double* weightData = weightArray + (vertex*4);

					// ignore weights and indices over 4 (optimal vertex-to-joint ratio for games is 4)
					if (stride > 3) continue;

					jointData[stride] = jointNode->GetJointIndex();
					weightData[stride] = weight;
					maxIndex = Math::n_max(jointData[stride], maxIndex);
					slotArray[vertex]++;
				}
			}
		}
	}

	// finally, traverse through every vertex and set their joint indices and weighs
	for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
	{
		MeshBuilderVertex& vertex = mesh->VertexAt(vertexIndex);
		int* indices = jointArray + (vertexIndex*4);
		double* weights = weightArray + (vertexIndex*4);

		// set weights
		vertex.SetComponent(MeshBuilderVertex::WeightsUB4NIndex, float4((float)weights[0], (float)weights[1], (float)weights[2], (float)weights[3]));

		// if we have more than 255 joints, use uncompressed joints
		if (maxIndex <= 255)	vertex.SetComponent(MeshBuilderVertex::JIndicesUB4Index, float4((float)indices[0], (float)indices[1], (float)indices[2], (float)indices[3]));
		else					vertex.SetComponent(MeshBuilderVertex::JIndicesIndex, float4((float)indices[0], (float)indices[1], (float)indices[2], (float)indices[3]));
	}

	delete [] slotArray;
	delete [] jointArray;
	delete [] weightArray;
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxMeshNode::GenerateRigidSkin()
{
	if (!(this->meshFlags & ToolkitUtil::HasSkin))
	{
		// parent to joint if it exists, otherwise just parent to root joint
		Ptr<NFbxNode> parent = this->GetParent();
		if (parent.isvalid() && parent->IsA(NFbxJointNode::RTTI))
		{
			Ptr<NFbxJointNode> parentJoint = parent.downcast<NFbxJointNode>();
			IndexT parentJointIndex = parentJoint->GetJointIndex();

			int vertCount = this->mesh->GetNumVertices();
			IndexT vertIndex;
			for (vertIndex = 0; vertIndex < vertCount; vertIndex++)
			{
				// for each vertex, set a rigid skin by using a weight of 1 and the index of the parent node
				MeshBuilderVertex& vertex = mesh->VertexAt(vertIndex);
				vertex.SetComponent(MeshBuilderVertex::WeightsUB4NIndex, float4(1.0f, 0.0f, 0.0f, 0.0f));
				vertex.SetComponent(MeshBuilderVertex::JIndicesUB4Index, float4((scalar)parentJointIndex, 0, 0, 0));
			}
		}
		else
		{
			int vertCount = this->mesh->GetNumVertices();
			IndexT vertIndex;
			for (vertIndex = 0; vertIndex < vertCount; vertIndex++)
			{
				// for each vertex, set a rigid skin by using a weight of 1 and the index of the parent node
				MeshBuilderVertex& vertex = mesh->VertexAt(vertIndex);
				vertex.SetComponent(MeshBuilderVertex::WeightsUB4NIndex, float4(1.0f, 0.0f, 0.0f, 0.0f));
				vertex.SetComponent(MeshBuilderVertex::JIndicesUB4Index, float4(0, 0, 0, 0));
			}
		}
		
		// set skin flag
		uint flags = this->meshFlags;
		flags |= ToolkitUtil::HasSkin;
		this->meshFlags = (ToolkitUtil::MeshFlags)flags;
	}

}

//------------------------------------------------------------------------------
/**
	Unparents transformations
*/
void 
NFbxMeshNode::DoMerge( Util::Dictionary<Util::String, Util::Array<Ptr<NFbxMeshNode> > >& meshes )
{
	NFbxNode::DoMerge(meshes);

	// set material to physics if we are physics
	Ptr<NFbxNode> parent = this->GetParent();
	if (parent.isvalid() && parent->IsPhysics())
	{
		this->material = "physics";

		// also remove all skin fragments
		this->skinFragments.Clear();
	}

	// make sure lods doesn't get merged
	if (this->lod != NULL)
	{
		String lodMaterial;
		lodMaterial.Format("_lod_%d_%d", this->lodIndex, this->name.HashCode());
		this->material.Append(lodMaterial);
	}

	// add this mesh to mesh dictionary, create entry if non-existent
	if (meshes.Contains(this->material))
	{
		n_assert(meshes[this->material].FindIndex(this) == InvalidIndex);
		meshes[this->material].Append(this);
	}
	else
	{
		// in this case we have a new material, and so we create a new list of nodes
		Array<Ptr<NFbxMeshNode> > nodeList;
		nodeList.Append(this);
		meshes.Add(this->material, nodeList);
	}
}

//------------------------------------------------------------------------------
/**
*/
const float 
NFbxMeshNode::GetLODMaxDistance() const
{
	n_assert(this->lod != NULL);
	FbxDistance dist;
	int index = this->lodIndex;
	bool hasMax = this->lod->GetThreshold(index, dist);

	float scale = this->scene->GetScale();
	if (hasMax)
	{
		return dist.value() * scale;
	}
	else
	{
		// return float max if there is no max-value
		return FLT_MAX;
	}
}

//------------------------------------------------------------------------------
/**
*/
const float 
NFbxMeshNode::GetLODMinDistance() const
{
	n_assert(this->lod != NULL);
	FbxDistance dist;
	int index = this->lodIndex - 1;
	bool hasMin = this->lod->GetThreshold(index, dist);

	float scale = this->scene->GetScale();
	if (hasMin)
	{
		return dist.value() * scale;
	}
	else
	{
		// return 0 if there is no min-value
		return 0.0f;
	}
}
} // namespace ToolkitUtil