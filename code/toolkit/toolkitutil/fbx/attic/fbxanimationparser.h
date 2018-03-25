#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FBXAnimationParser
    
    Searches an FBX scene to find animations
    
    (C) 2012 gscept
*/
#include "toolkitutil/animutil/animbuilderclip.h"
#include "fbxparserbase.h"
#include "fbxtypes.h"
#include "splitter/animsplitterhelper.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class FBXAnimationParser : public FBXParserBase
{
	__DeclareClass(FBXAnimationParser);
public:

	enum ParseMode
	{
		Joints,
		Static,

		NumParseModes
	};
	/// constructor
	FBXAnimationParser();
	/// destructor
	virtual ~FBXAnimationParser();

	/// parses a scene in search of animations
	void Parse(KFbxScene* scene, ToolkitUtil::AnimBuilder* animBuilder = 0);

	/// sets the animation splitter
	void SetAnimSplitter(const Ptr<AnimSplitterHelper>& animSplitter);
	/// sets what skeleton should be used to extract animations
	void SetSkeleton(Skeleton* skeleton);
	/// gets the skeleton
	const Skeleton* GetSkeleton() const;

	/// sets the parse mode
	void SetParseMode(ParseMode mode);
private:
	/// recursively traverses a skeleton and finds animation curves.
	void ConstructAnimationCurvesFromSkeleton(KFbxNode* fbxNode, KFbxAnimLayer* fbxAnimLayer, Util::Array<ToolkitUtil::AnimBuilderCurve>& curves, int& preInfType, int& postInfType, int keySpan);
	/// recursively traverses scene to locate animated transform nodes
	void ConstructAnimationCurvesFromScene(KFbxScene* scene, KFbxAnimLayer* fbxAnimLayer, Util::Array<ToolkitUtil::AnimBuilderCurve>& curves, int& preInfType, int& postInfType, int keySpan);
	/// recursively parses the skeleton and finds the longest animation key
	void GetMaximumKeySpanFromSkeleton(KFbxNode* fbxNode, KFbxAnimLayer* fbxAnimLayer, int& keySpan);
	/// converts FBX time mode to FPS
	float TimeModeToFPS(const KTime::ETimeMode& timeMode);
	/// reads a single clip
	void ReadClip(KFbxNode* fbxNode, KFbxAnimLayer* fbxAnimLayer, Util::Array<ToolkitUtil::AnimBuilderCurve>& curves, int& preInfType, int& postInfType, int keySpan);

	ParseMode mode;
	Skeleton* skeleton;
	Ptr<AnimSplitterHelper> splitter;

	float currentScaleFactor;
}; 


//------------------------------------------------------------------------------
/**
*/
inline void 
FBXAnimationParser::SetAnimSplitter( const Ptr<AnimSplitterHelper>& animSplitter )
{
	this->splitter = animSplitter;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FBXAnimationParser::SetSkeleton( Skeleton* skeleton )
{
	this->skeleton = skeleton;
}

//------------------------------------------------------------------------------
/**
*/
inline const Skeleton* 
FBXAnimationParser::GetSkeleton() const
{
	n_assert(this->skeleton);
	return this->skeleton;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
FBXAnimationParser::SetParseMode( ParseMode mode )
{
	this->mode = mode;
}
} // namespace ToolkitUtil
//------------------------------------------------------------------------------