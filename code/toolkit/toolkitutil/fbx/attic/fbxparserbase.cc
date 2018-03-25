//------------------------------------------------------------------------------
//  fbxparserbase.cc
//  (C) 2011 gscept
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fbxparserbase.h"
#include "base\exporterbase.h"

namespace ToolkitUtil
{
__ImplementAbstractClass(ToolkitUtil::FBXParserBase, 'FBXB', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
FBXParserBase::FBXParserBase() : 
	inParse(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FBXParserBase::~FBXParserBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXParserBase::BeginParse()
{
	n_assert(this->inParse);
	this->inParse = true;

}

//------------------------------------------------------------------------------
/**
*/
void 
FBXParserBase::EndParse()
{
	n_assert(!this->inParse);
	this->inParse = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXParserBase::Setup( KFbxSdkManager* manager )
{
	this->sdkManager = manager;
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXParserBase::Parse( KFbxScene* scene, AnimBuilder* animBuilder /* = 0 */ )
{
	// override this!
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXParserBase::SetExporter( const Ptr<Base::ExporterBase>& exporter )
{
	this->exporter = exporter;
}

} // namespace ToolkitUtil