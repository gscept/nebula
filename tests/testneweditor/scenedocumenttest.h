#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::SceneDocumentRefTest

    The entity references inside extension data, as the document knows them. The document reads an
    extension's text as bytes it cannot interpret, so the reference hooks are the only way it can
    record a reference in one, and the only way it can put another file's guids into it. This is the
    document's side of that contract, with the smallest extension that can answer for its own text.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class SceneDocumentRefTest : public TestCase
{
    __DeclareClass(SceneDocumentRefTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
