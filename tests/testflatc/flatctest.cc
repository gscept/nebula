//------------------------------------------------------------------------------
//  flatctest.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "flatctest.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"
#include "flatbuffers/registry.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/tests/flatc_test.h"
namespace Test
{
__ImplementClass(Test::FlatCTest, 'flct', Test::TestCase);

using namespace System;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
FlatCTest::Run()
{
    ItemT testItem;
    testItem.name = "ItemName";
    testItem.pos = Math::vec4(1, 2, 3, 4);

    flatbuffers::FlatBufferBuilder builder;
    builder.Finish(Item::Pack(builder, &testItem));

    std::string buf;
    flatbuffers::LoadFile("flatc_test.fbs", false, &buf);
    flatbuffers::Parser parser;
    parser.Parse(buf.c_str());
    std::string output;
    flatbuffers::GenerateText(parser, builder.GetBufferPointer(), &output);

    n_printf("output:\n%s", output.c_str()); 



}

} // namespace Test