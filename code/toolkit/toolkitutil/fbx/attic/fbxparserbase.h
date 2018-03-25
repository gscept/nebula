#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FBXParserBase
    
    Base class for parsers
    
    (C) 2012 gscept
*/
#include "core/refcounted.h"
#include "toolkitutil/meshutil/meshbuilder.h"
#include "toolkitutil/animutil/animbuilder.h"
#include <fbxsdk.h>

namespace Base
{
	class ExporterBase;
}
//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class FBXParserBase : public Core::RefCounted
{
	__DeclareAbstractClass(FBXParserBase);
public:
	/// constructor
	FBXParserBase();
	/// destructor
	virtual ~FBXParserBase();

	/// sets the FBX SDK context
	void Setup(KFbxSdkManager* manager);
	/// parses a model scene
	virtual void Parse(KFbxScene* scene, ToolkitUtil::AnimBuilder* animBuilder = 0) = 0;
	
	/// sets the parent exporter (used to report progress)
	void SetExporter(const Ptr<Base::ExporterBase>& exporter);

	/// sets the scale of the scene
	void SetScale(float scale);

	/// begins a parse cycle
	virtual void BeginParse();
	/// ends a parse cycle
	virtual void EndParse();

	/// sets increment
	void SetIncrement(int increment);

protected:

	int increment;

	Ptr<Base::ExporterBase> exporter;
	KFbxSdkManager* sdkManager;
	bool inParse;
	float scaleFactor;

}; 


//------------------------------------------------------------------------------
/**
*/
inline void 
FBXParserBase::SetScale( float scale )
{
	this->scaleFactor = scale;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
FBXParserBase::SetIncrement( int increment )
{
	this->increment = increment;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------