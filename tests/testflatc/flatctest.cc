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
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/tests/flatc_test.h"
#include "io/ioserver.h"

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
    Ptr<IO::IoServer> io = IO::IoServer::Create();
    ItemT testItem;
    testItem.name = "ItemName";
    testItem.pos = Math::vec4(1, 2, 3, 4);

    Flat::FlatbufferInterface::Init();
    Util::String jsonString = SerializeFlatbufferText(Item, testItem);
    // TODO find good test for json output

    flatbuffers::FlatBufferBuilder builder;
    builder.Finish(Item::Pack(builder, &testItem));

    Util::Blob buf = Flat::FlatbufferInterface::SerializeFlatbuffer<Item>(testItem);

    ItemT test2Item;
    Flat::FlatbufferInterface::DeserializeFlatbuffer<Item>(test2Item, (uint8_t*) buf.GetPtr());

    VERIFY(test2Item.name == testItem.name);
    VERIFY(test2Item.pos == testItem.pos);
}

} // namespace Test