#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FBXSkeletonParser
    
    Searches an FBX file to find skeletons
    
    (C) 2012 gscept
*/
#include "fbxparserbase.h"
#include "toolkitutil/n3util/n3modeldata.h"
#include "fbxtypes.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{


class FBXSkeletonParser : public FBXParserBase
{
	__DeclareClass(FBXSkeletonParser);
public:
	/// constructor
	FBXSkeletonParser();
	/// destructor
	virtual ~FBXSkeletonParser();

	/// parses a scene in search of skeletons
	void Parse(KFbxScene* scene, ToolkitUtil::AnimBuilder* animBuilder = 0);

	/// cleans up skeletons
	void Cleanup();

	/// gets the skeleton list after a parse
	const Util::Array<Skeleton*> GetSkeletons() const;
private:
	/// constructs a node tree depth-first
	void ConstructJointTree(Util::Array<SkeletonJoint*>& joints, KFbxNode* fbxNode, int& currentIndex, int parentIndex);

	Util::Array<Skeleton*> skeletons;
}; 


//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Skeleton*> 
FBXSkeletonParser::GetSkeletons() const
{
	return this->skeletons;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------