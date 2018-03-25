//------------------------------------------------------------------------------
//  fbxmodelparser.cc
//  (C) 2011 gscept
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fbxmodelparser.h"
#include "fbxskinparser.h"
#include "fbxparserbase.h"
#include "base/exporterbase.h"

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::FBXModelParser, 'FBXM', ToolkitUtil::FBXParserBase);

using namespace Util;
using namespace ToolkitUtil;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
FBXModelParser::FBXModelParser() : 
	staticMeshBuilder(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FBXModelParser::~FBXModelParser()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXModelParser::Cleanup()
{
	if (this->staticMeshBuilder)
	{
		this->staticMeshBuilder->Clear();
		delete this->staticMeshBuilder;
		this->staticMeshBuilder = 0;
	}

	for (int meshIndex = 0; meshIndex < this->meshes.Size(); meshIndex++)
	{
		delete this->meshes[meshIndex];
	}
	for (int skinIndex = 0; skinIndex < this->skinnedMeshes.Size(); skinIndex++)
	{
		delete this->skinnedMeshes[skinIndex]->meshSource;
		delete this->skinnedMeshes[skinIndex];
	}
	this->skinnedMeshes.Clear();
	this->meshes.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXModelParser::Parse( KFbxScene* scene, AnimBuilder* animBuilder /* = 0 */ )
{
	this->staticMeshBuilder = new MeshBuilder();
	MeshBuilder* meshBuilder = staticMeshBuilder;

	int nodeCount = scene->GetSrcObjectCount(FBX_TYPE(KFbxMesh));
	for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
	{
		KFbxMesh* fbxMesh = scene->GetSrcObject(FBX_TYPE(KFbxMesh), nodeIndex);

		ShapeNode* mesh = new ShapeNode();
		mesh->fbxNode = fbxMesh->GetNode();
		mesh->name = this->GetUniqueNodeName(fbxMesh->GetNode()->GetName());

		n_printf("Exporting mesh: %s \n", fbxMesh->GetNode()->GetName() );
		this->exporter->Progress(0, "Exporting: " + String(fbxMesh->GetNode()->GetName()));

		bool isSkinned;
		// if our mesh is a skin, we will treat it as a new skinned mesh
		if (fbxMesh->GetDeformerCount(KFbxDeformer::eSKIN) > 0 && skeletons.Size() > 0)
		{
			mesh->primGroup = this->skinnedMeshes.Size();
			mesh->isSkinned = true;			
			this->skinnedMeshes.Append(mesh);
			isSkinned = true;
		}
		else
		{
			mesh->primGroup = this->meshes.Size();
			mesh->isSkinned = false;
			this->meshes.Append(mesh);
			isSkinned = false;
		}

		fbxDouble3 rotation = fbxMesh->GetNode()->LclRotation.Get();
		fbxDouble3 translation = fbxMesh->GetNode()->LclTranslation.Get();

		mesh->rotation = quaternion::rotationyawpitchroll(n_deg2rad((float)rotation[0]), n_deg2rad((float)rotation[1]), n_deg2rad((float)rotation[2]));
		mesh->translation = vector((float)translation[0], (float)translation[1], (float)translation[2]);
		
		// exportation fail safe, we want to make sure our mesh is triangulated before going any further
		if (!fbxMesh->IsTriangleMesh())
		{
			KFbxGeometryConverter* converter = new KFbxGeometryConverter(scene->GetFbxSdkManager());
			fbxMesh = converter->TriangulateMesh(fbxMesh);
			delete converter;
		}

		int vertexCount = fbxMesh->GetControlPointsCount();
		int polyCount = fbxMesh->GetPolygonCount();
		int uvCount = fbxMesh->GetUVLayerCount();
		int normalCount = fbxMesh->GetElementNormalCount();
		int binormalCount = fbxMesh->GetElementBinormalCount();
		int tangentCount = fbxMesh->GetElementTangentCount();

		int vertexOffset = meshBuilder->GetNumVertices();
		int triangleOffset = meshBuilder->GetNumTriangles();

		MeshBuilder* currentMesh = new MeshBuilder();
		mesh->meshSource = currentMesh;

		// setup vertices with basic per-vertex data
		for (int vertex = 0; vertex < vertexCount; vertex++)
		{
			KFbxVector4 v = fbxMesh->GetControlPointAt(vertex);
			MeshBuilderVertex meshVertex;
			float4 position = float4((float)v[0] / this->scaleFactor,(float)v[1] / this->scaleFactor,(float)v[2] / this->scaleFactor, 1.0f);
			
			meshVertex.SetComponent(MeshBuilderVertex::CoordIndex, position);
			currentMesh->AddVertex(meshVertex);
		}

		// setup triangles
		for (int polygonIndex = 0; polygonIndex < polyCount; polygonIndex++)
		{
			int polygonSize = fbxMesh->GetPolygonSize(polygonIndex);
			n_assert(polygonSize == 3);
			MeshBuilderTriangle meshTriangle;
			meshTriangle.SetGroupId(mesh->primGroup);
			for (int polygonVertexIndex = 0; polygonVertexIndex < polygonSize; polygonVertexIndex++)
			{
				// we want to offset the vertex index with the current size of the mesh
				int polygonVertex = fbxMesh->GetPolygonVertex(polygonIndex, polygonVertexIndex);
				meshTriangle.SetVertexIndex(polygonVertexIndex, polygonVertex);
			}
			currentMesh->AddTriangle(meshTriangle);
		}

		if (isSkinned)
		{
			Ptr<FBXSkinParser> skinParser = FBXSkinParser::Create();
			skinParser->SetExporter(this->exporter);
			skinParser->SetMesh(mesh);
			skinParser->SetSkeletonList(skeletons);
			skinParser->Parse(scene, animBuilder);
		}
		

		// inflate the mesh to prepare for vertex copying
		currentMesh->Inflate();		

		int vertexId = 0;
		for (int triangleIndex = 0; triangleIndex < polyCount; triangleIndex++)
		{
			MeshBuilderTriangle& triangle = currentMesh->TriangleAt(triangleIndex);
			for (int polygonVertexIndex = 0; polygonVertexIndex < 3; polygonVertexIndex++)
			{
				int polygonVertex = fbxMesh->GetPolygonVertex(triangleIndex, polygonVertexIndex);
				int inflatedIndex = triangle.GetVertexIndex(polygonVertexIndex);
				MeshBuilderVertex& vertexRef = currentMesh->VertexAt(inflatedIndex);

				for (int uv = 0; uv < uvCount; uv++)
				{
					this->ParseUVs(fbxMesh, triangleIndex, polygonVertexIndex, polygonVertex, uv, vertexRef);
				}

				this->ParseNormals(fbxMesh, polygonVertex, vertexId, vertexRef);
				vertexId++;
			}
		}

		// deflate to remove redundant vertices
		currentMesh->Deflate(0);

		// compute a bounding box for the deflated mesh
		mesh->boundingBox = currentMesh->ComputeBoundingBox();


		// apparently we need to do this...
// 		if (!fbxMesh->CheckIfVertexNormalsCCW())
// 		{
// 		}

		this->CalculateTangentsAndBinormals(*currentMesh);
		currentMesh->FlipUvs();

		// if the mesh is skinned, we want to preserve our new mesh, otherwise we just want the static one
		if (isSkinned)
		{
			mesh->meshSource = currentMesh;
		}
		else
		{
			int newMeshVertexCount = currentMesh->GetNumVertices();
			int newMeshTriangleCount = currentMesh->GetNumTriangles();

			for (int triIndex = 0; triIndex < newMeshTriangleCount; triIndex++)
			{
				MeshBuilderTriangle& tri = currentMesh->TriangleAt(triIndex);
				tri.SetVertexIndex(0, vertexOffset + tri.GetVertexIndex(0));
				tri.SetVertexIndex(1, vertexOffset + tri.GetVertexIndex(1));
				tri.SetVertexIndex(2, vertexOffset + tri.GetVertexIndex(2));
				staticMeshBuilder->AddTriangle(currentMesh->TriangleAt(triIndex));
			}

			for (int vertIndex = 0; vertIndex < newMeshVertexCount; vertIndex++)
			{
				staticMeshBuilder->AddVertex(currentMesh->VertexAt(vertIndex));
			}

			currentMesh->Clear();

			mesh->meshSource = staticMeshBuilder;
			delete currentMesh;
		}


		n_printf("Mesh done!\n");

	}


	// calculate tangents and bitangents for the first UV-set (so remember to keep the diffuse map and normal map in the same UV-map)
	//this->CalculateTangentsAndBinormals(*meshBuilder);



}

//------------------------------------------------------------------------------
/**
	Handles fetching of UV information based on a number of different modes depending on how the mesh was saved by the exporter
*/
void 
FBXModelParser::ParseUVs( KFbxMesh* mesh, int polyIndex, int polyVertexIndex, int polyVertex, int layer, MeshBuilderVertex& vertexRef )
{
	if (layer > 3)
	{
		n_warning("Multilayered texturing only supports up to four layers!");
	}
	KFbxGeometryElementUV* uvElement = mesh->GetElementUV(layer);
	String uvName = uvElement->GetName();
	switch(uvElement->GetMappingMode())
	{
	case KFbxGeometryElement::eBY_POLYGON_VERTEX:
		switch (uvElement->GetReferenceMode())
		{
		case KFbxGeometryElement::eDIRECT:
		case KFbxGeometryElement::eINDEX_TO_DIRECT:
			{
				int textureUVIndex = mesh->GetTextureUVIndex(polyIndex, polyVertexIndex);
				KFbxVector2 u = uvElement->GetDirectArray().GetAt(textureUVIndex);
				vertexRef.SetComponent((MeshBuilderVertex::ComponentIndex)(MeshBuilderVertex::Uv0Index+layer*2), float4((float)u[0],(float)u[1],0.0f,0.0f));
				break;
			}
		default:
			n_error("UV-coordinates has to be either direct, or indexed to direct");
			break;
		} 
		break;
	case KFbxGeometryElement::eBY_CONTROL_POINT:
		switch (uvElement->GetReferenceMode())
		{
		case KFbxGeometryElement::eDIRECT:
			{
				KFbxVector2 u = uvElement->GetDirectArray().GetAt(polyVertex);
				vertexRef.SetComponent((MeshBuilderVertex::ComponentIndex)(MeshBuilderVertex::Uv0Index+layer*2), float4((float)u[0],(float)u[1],0.0f,0.0f));
				break;
			}
		case KFbxGeometryElement::eINDEX_TO_DIRECT:
			{
				KFbxVector2 u = uvElement->GetDirectArray().GetAt(uvElement->GetIndexArray().GetAt(polyVertex));
				vertexRef.SetComponent((MeshBuilderVertex::ComponentIndex)(MeshBuilderVertex::Uv0Index+layer*2), float4((float)u[0],(float)u[1],0.0f,0.0f));
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
FBXModelParser::ParseNormals( KFbxMesh* mesh, int polyVertex, int vertexId, MeshBuilderVertex& vertexRef )
{
	KFbxGeometryElementNormal* normalElement = mesh->GetElementNormal(0);
	switch(normalElement->GetMappingMode())
	{
	case KFbxGeometryElement::eBY_POLYGON_VERTEX:
		switch (normalElement->GetReferenceMode())
		{
		case KFbxGeometryElement::eDIRECT:
			{
				KFbxVector4 n = normalElement->GetDirectArray().GetAt(vertexId);
				if (vertexRef.HasComponent(MeshBuilderVertex::NormalBit))
				{
					float4 oldNormal = vertexRef.GetComponent(MeshBuilderVertex::NormalIndex);
					float4 newNormal = float4::normalize(float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.0f));
					vertexRef.SetComponent(MeshBuilderVertex::NormalIndex, newNormal);
				}
				else
				{
					vertexRef.SetComponent(MeshBuilderVertex::NormalIndex, float4((float)n[0],(float)n[1],(float)n[2], 0.0f));
				}
				break;
			}
		case KFbxGeometryElement::eINDEX_TO_DIRECT:
			{
				KFbxVector4 n = normalElement->GetDirectArray().GetAt(normalElement->GetIndexArray().GetAt(vertexId));
				if (vertexRef.HasComponent(MeshBuilderVertex::NormalBit))
				{
					float4 oldNormal = vertexRef.GetComponent(MeshBuilderVertex::NormalIndex);
					float4 newNormal = float4::normalize(float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.0f));
					vertexRef.SetComponent(MeshBuilderVertex::NormalIndex, newNormal);
				}
				else
				{
					vertexRef.SetComponent(MeshBuilderVertex::NormalIndex, float4((float)n[0],(float)n[1],(float)n[2], 0.0f));
				}
				break;
			}
		default:
			n_error("Normals has to be either direct, or indexed to direct");
			break;
		}
		break;
	case KFbxGeometryElement::eBY_CONTROL_POINT:
		switch (normalElement->GetReferenceMode())
		{
		case KFbxGeometryElement::eDIRECT:
			{
				KFbxVector4 n = normalElement->GetDirectArray().GetAt(polyVertex);
				if (vertexRef.HasComponent(MeshBuilderVertex::NormalBit))
				{
					float4 oldNormal = vertexRef.GetComponent(MeshBuilderVertex::NormalIndex);
					float4 newNormal = float4::normalize(float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.0f));
					vertexRef.SetComponent(MeshBuilderVertex::NormalIndex, newNormal);
				}
				else
				{
					vertexRef.SetComponent(MeshBuilderVertex::NormalIndex, float4((float)n[0],(float)n[1],(float)n[2], 0.0f));
				}
				break;
			}
		case KFbxGeometryElement::eINDEX_TO_DIRECT:
			{
				KFbxVector4 n = normalElement->GetDirectArray().GetAt(normalElement->GetIndexArray().GetAt(polyVertex));
				if (vertexRef.HasComponent(MeshBuilderVertex::NormalBit))
				{
					float4 oldNormal = vertexRef.GetComponent(MeshBuilderVertex::NormalIndex);
					float4 newNormal = float4::normalize(float4(oldNormal.x() + (float)n[0], oldNormal.y() + (float)n[1], oldNormal.z() + (float)n[2], 0.0f));
					vertexRef.SetComponent(MeshBuilderVertex::NormalIndex, newNormal);
				}
				else
				{
					vertexRef.SetComponent(MeshBuilderVertex::NormalIndex, float4((float)n[0],(float)n[1],(float)n[2], 0.0f));
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
FBXModelParser::CalculateNormals( ToolkitUtil::MeshBuilder& mesh )
{
	int numTriangles = mesh.GetNumTriangles();
	for (int triIndex = 0; triIndex < numTriangles; triIndex++)
	{
		MeshBuilderTriangle& tri = mesh.TriangleAt(triIndex);
		int vertIndex1, vertIndex2, vertIndex3;
		tri.GetVertexIndices(vertIndex1, vertIndex2, vertIndex3);

		MeshBuilderVertex& v1 = mesh.VertexAt(vertIndex1);
		MeshBuilderVertex& v2 = mesh.VertexAt(vertIndex2);
		MeshBuilderVertex& v3 = mesh.VertexAt(vertIndex3);

		float4 p1 = v1.GetComponent(MeshBuilderVertex::CoordIndex);
		float4 p2 = v2.GetComponent(MeshBuilderVertex::CoordIndex);
		float4 p3 = v3.GetComponent(MeshBuilderVertex::CoordIndex);

		vector normal1 = float4::cross3(p2 - p1, p3 - p1);
		vector normal2 = float4::cross3(p3 - p2, p1 - p2);
		vector normal3 = float4::cross3(p1 - p3, p2 - p3);

		if (v1.HasComponent(MeshBuilderVertex::NormalBit))
		{
			v1.SetComponent(MeshBuilderVertex::NormalIndex, v1.GetComponent(MeshBuilderVertex::NormalIndex) + normal1);
		}
		else
		{
			v1.SetComponent(MeshBuilderVertex::NormalIndex, normal1);
		}

		if (v2.HasComponent(MeshBuilderVertex::NormalBit))
		{
			v2.SetComponent(MeshBuilderVertex::NormalIndex, v2.GetComponent(MeshBuilderVertex::NormalIndex) + normal2);
		}
		else
		{
			v2.SetComponent(MeshBuilderVertex::NormalIndex, normal2);
		}

		if (v3.HasComponent(MeshBuilderVertex::NormalBit))
		{
			v3.SetComponent(MeshBuilderVertex::NormalIndex, v3.GetComponent(MeshBuilderVertex::NormalIndex) + normal3);
		}
		else
		{
			v3.SetComponent(MeshBuilderVertex::NormalIndex, normal3);
		}
	}
}

//------------------------------------------------------------------------------
/**
	Calculates binormals and tangents based on one UV-set, so use this set for your normal maps as well
*/
void 
FBXModelParser::CalculateTangentsAndBinormals( MeshBuilder& mesh )
{

	int numVertices = mesh.GetNumVertices();
	int numTriangles = mesh.GetNumTriangles();
	float4* tangents1 = new float4[numVertices * 2];
	float4* tangents2 = tangents1 + numVertices;

	memset(tangents1, 0, numVertices * sizeof(float4) * 2);
	for (int triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
	{
		MeshBuilderTriangle& triangle = mesh.TriangleAt(triangleIndex);
		int v1Index = triangle.GetVertexIndex(0);
		int v2Index = triangle.GetVertexIndex(1);
		int v3Index = triangle.GetVertexIndex(2);

		MeshBuilderVertex& vertex1 = mesh.VertexAt(v1Index);
		MeshBuilderVertex& vertex2 = mesh.VertexAt(v2Index);
		MeshBuilderVertex& vertex3 = mesh.VertexAt(v3Index);

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
		MeshBuilderVertex& vertex = mesh.VertexAt(vertexIndex);
		float4 n = float4::normalize(vertex.GetComponent(MeshBuilderVertex::NormalIndex));
		float4 t = tangents1[vertexIndex];
		float4 b = tangents2[vertexIndex];

		float4 tangent = float4::normalize(t - n * float4::dot3(n, t));
		float handedNess = (float4::dot3(float4::cross3(n, t), tangents2[vertexIndex]) < 0.0f ? 1.0f : -1.0f);
		float4 bitangent = float4::normalize(float4::cross3(n, tangent) * handedNess);

		if (vertex.HasComponent(MeshBuilderVertex::TangentBit))
		{
			float4 oldTangent = vertex.GetComponent(MeshBuilderVertex::TangentIndex);
			float4 newTangent = oldTangent + tangent;
			vertex.SetComponent(MeshBuilderVertex::TangentIndex, newTangent);
		}
		else
		{
			vertex.SetComponent(MeshBuilderVertex::TangentIndex, tangent);
		}

		if (vertex.HasComponent(MeshBuilderVertex::BinormalBit))
		{
			float4 oldBinormal = vertex.GetComponent(MeshBuilderVertex::BinormalIndex);
			float4 newBinormal = oldBinormal + bitangent;
			vertex.SetComponent(MeshBuilderVertex::BinormalIndex, newBinormal);
		}
		else
		{
			vertex.SetComponent(MeshBuilderVertex::BinormalIndex, bitangent);
		}
	}

	// finally normalize all of it!
	for (int vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
	{
		MeshBuilderVertex& vertex = mesh.VertexAt(vertexIndex);
		vertex.SetComponent(MeshBuilderVertex::NormalIndex, float4::normalize(vertex.GetComponent(MeshBuilderVertex::NormalIndex)));
		vertex.SetComponent(MeshBuilderVertex::TangentIndex, float4::normalize(vertex.GetComponent(MeshBuilderVertex::TangentIndex)));
		vertex.SetComponent(MeshBuilderVertex::BinormalIndex, float4::normalize(vertex.GetComponent(MeshBuilderVertex::BinormalIndex)));
	}

	delete [] tangents1;
	
}

//------------------------------------------------------------------------------
/**
*/
const Util::String 
FBXModelParser::GetUniqueNodeName( const Util::String& origName )
{
	MeshList totalList;
	totalList.AppendArray(this->skinnedMeshes);
	totalList.AppendArray(this->meshes);

	bool found = false;
	int uniqueCounter = 0;
	Util::String retString = origName;
	for (int meshIndex = 0; meshIndex < totalList.Size(); meshIndex++)
	{
		if (retString == totalList[meshIndex]->name)
		{
			retString = origName + "_" + String::FromInt(uniqueCounter++);
		}		
	}

	return retString;
}

} // namespace ToolkitUtil