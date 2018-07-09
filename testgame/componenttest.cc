//------------------------------------------------------------------------------
//  componenttest.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "componenttest.h"
#include "ids/id.h"
#include "component/componentcontainer.h"
#include "math/float4.h"
#include "util/string.h"

using namespace Game;

namespace Test
{
__ImplementClass(Test::CompDataTest, 'CDTS', Test::TestCase);

typedef struct 
{
    Util::String name;
    Math::float4 pos;
} testdata;

//------------------------------------------------------------------------------
/**
*/
void
CompDataTest::Run()
{
    IdSystem id;
    ComponentDataContainer<testdata> comp;
    Id tmp = id.Allocate();
    comp.Allocate(tmp.Index());
    //uint32_t iid = comp.GetInstance(tmp);
    //testdata & data = comp.GetInstanceData(iid);


}
}