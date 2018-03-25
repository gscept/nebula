#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FBXSkinParser
    
    Searches an FBX scene and extracts skins
    
    (C) 2012 gscept
*/
#include "fbxparserbase.h"
#include "fbxtypes.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class FBXSkinParser : public FBXParserBase
{
	__DeclareClass(FBXSkinParser);
public:

	/// constructor
	FBXSkinParser();
	/// destructor
	virtual ~FBXSkinParser();

	/// parses a scene in search of skins
	void Parse(KFbxScene* scene, ToolkitUtil::AnimBuilder* animBuilder = 0);

	/// sets a list of skeletons to be used for skin->skeleton connections
	void SetSkeletonList(SkeletonList skeletons);

	/// sets the mesh which should get skinned
	void SetMesh(ShapeNode* mesh);

private:
	ShapeNode* mesh;
	SkeletonList skeletons;
}; 


//------------------------------------------------------------------------------
/**
*/
inline void 
FBXSkinParser::SetSkeletonList( SkeletonList skeletons )
{
	this->skeletons = skeletons;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FBXSkinParser::SetMesh( ShapeNode* mesh )
{
	n_assert(mesh);
	this->mesh = mesh;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------