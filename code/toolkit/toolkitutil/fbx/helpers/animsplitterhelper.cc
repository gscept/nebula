//------------------------------------------------------------------------------
//  animsplitterhelper.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "animsplitterhelper.h"

using namespace IO;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::AnimSplitterHelper, 'ASPH', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
AnimSplitterHelper::AnimSplitterHelper()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
AnimSplitterHelper::~AnimSplitterHelper()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimSplitterHelper::Setup( Ptr<XmlReader> reader )
{
	if (reader->SetToFirstChild()) do 
	{
		if (reader->GetCurrentNodeName() != "Take")
		{
			continue;
		}
		Util::String takeName = reader->GetString("name");
		Util::Array<Split> splits;
		if (reader->SetToFirstChild()) do 
		{
			Split split;
			split.name = reader->GetString("name");
			split.startOffset = reader->GetInt("start");
			split.endOffset = reader->GetInt("end");
			split.preInfinity = (CoreAnimation::InfinityType::Code)reader->GetInt("pre");
			split.postInfinity = (CoreAnimation::InfinityType::Code)reader->GetInt("post");

			splits.Append(split);
		} 
		while (reader->SetToNextChild());

		this->takes.Add(takeName, splits);
	} 
	while (reader->SetToNextChild());
}


} // namespace ToolkitUtil