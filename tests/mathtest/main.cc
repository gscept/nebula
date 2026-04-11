//------------------------------------------------------------------------------
//  mathtest/main.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "core/coreserver.h"
#include "testbase/testrunner.h"

// tests
#include "scalartest.h"
#include "halftest.h"
#include "vec3test.h"
#include "vec4test.h"
#include "mat4test.h"
#include "quaterniontest.h"
#include "vectortest.h"
#include "pointtest.h"
#include "planetest.h"
#include "bboxtest.h"
#include "testbase/stackdebug.h"

ImplementNebulaApplication();

using namespace Core;
using namespace Test;

//------------------------------------------------------------------------------
/**
*/
void
NebulaMain(const Util::CommandLineArgs& args)
{
    STACK_CHECKPOINT("NebulaMain");

    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName(Util::StringAtom("Nebula Foundation Math-Tests"));
    coreServer->Open();

    n_printf("NEBULA FOUNDATION MATH-TESTS\n");
    n_printf("========================\n");

    // setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();  
    testRunner->AttachTestCase(ScalarTest::Create());
    testRunner->AttachTestCase(HalfTest::Create());
    testRunner->AttachTestCase(Vec3Test::Create());
    testRunner->AttachTestCase(Vec4Test::Create());
    testRunner->AttachTestCase(Mat4Test::Create());
    testRunner->AttachTestCase(QuaternionTest::Create());
    testRunner->AttachTestCase(VectorTest::Create());
    testRunner->AttachTestCase(PointTest::Create());
    testRunner->AttachTestCase(PlaneTest::Create());
    testRunner->AttachTestCase(BBoxTest::Create());

    bool result = testRunner->Run(); 

    coreServer->Close();
    testRunner = nullptr;
    coreServer = nullptr;
    
    Core::SysFunc::Exit(result ? 0 : -1);
}
