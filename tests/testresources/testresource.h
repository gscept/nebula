#pragma once
//------------------------------------------------------------------------------
/**
	Test resource, does nothing for us really :(
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resource.h"
#include "util/string.h"
namespace Test
{

RESOURCE_ID_TYPE(TestResourceId);

enum IdTypes
{
	TestResourceIdType
};

struct TestResourceData
{
	Util::String data;
};
} // namespace Test