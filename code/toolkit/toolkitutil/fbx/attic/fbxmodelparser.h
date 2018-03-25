#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FBXModelParser
    
    Parses an FBX scene in search of models
    
    (C) 2012 gscept
*/
#include "fbxparserbase.h"
#include "fbxtypes.h"
#include "ToolkitUtil\meshutil\meshbuildervertex.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class FBXModelParser : public FBXParserBase
{
	__DeclareClass(FBXModelParser);
public:

	/// constructor
	FBXModelParser();
	/// destructor
	virtual ~FBXModelParser();

	/// parses a scene in search of models
	void Parse(KFbxScene* scene, ToolkitUtil::AnimBuilder* animBuilder = 0);
	/// gets the meshes from the FBX model parser
	const MeshList& GetMeshes() const;
	/// gets the skinned meshes from the FBX model parser
	const MeshList& GetSkinnedMeshes() const;
	/// sets the skeletons to be used when skinning
	void SetSkeletons(const SkeletonList& skeletons);

	/// cleans up any undeleted mesh builders
	void Cleanup();

private:
	/// parses uvs
	void ParseUVs(KFbxMesh* mesh, int polyIndex, int polyVertexIndex, int polyVertex, int layer, ToolkitUtil::MeshBuilderVertex& vertexRef);
	/// parses normals
	void ParseNormals(KFbxMesh* mesh, int polyVertex, int vertexId, ToolkitUtil::MeshBuilderVertex& vertexRef);
	/// evaulates if a triangle is CCW
	void EnsureNormalsCW(const ToolkitUtil::MeshBuilderTriangle& tri, const ToolkitUtil::MeshBuilder& mesh);

	/// calculates normals
	void CalculateNormals(ToolkitUtil::MeshBuilder& mesh);
	/// calculates binormals and tangents using normal
	void CalculateTangentsAndBinormals(ToolkitUtil::MeshBuilder& mesh);

	/// ensures a mesh gets a unique name
	const Util::String GetUniqueNodeName(const Util::String& origName);

	MeshList meshes;
	MeshList skinnedMeshes;
	SkeletonList skeletons;

	ToolkitUtil::MeshBuilder* staticMeshBuilder;
}; 



//------------------------------------------------------------------------------
/**
*/
inline const MeshList& 
FBXModelParser::GetMeshes() const
{
	return this->meshes;
}

//------------------------------------------------------------------------------
/**
*/
inline const MeshList& 
FBXModelParser::GetSkinnedMeshes() const
{
	return this->skinnedMeshes;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FBXModelParser::SetSkeletons( const SkeletonList& skeletons )
{
	this->skeletons = skeletons;
}



} // namespace ToolkitUtil
//------------------------------------------------------------------------------