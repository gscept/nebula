//------------------------------------------------------------------------------
//  cvartest.cc
// (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "cvartest.h"
#include "core/cvar.h"

namespace Test
{
__ImplementClass(Test::CVarTest, 'CVTS' , Test::TestCase);

using namespace Core;

//------------------------------------------------------------------------------
/**
*/
void
CVarTest::Run()
{
    CVarCreateInfo intInfo;
    intInfo.name = "testInt";
    intInfo.defaultValue = "10";
    intInfo.type = CVar_Int;
    CVar* testInt = CVarCreate(intInfo);
    VERIFY(10 == CVarReadInt(testInt));
    VERIFY(0 == CVarReadFloat(testInt));
    VERIFY(0 == CVarReadString(testInt));

    CVarCreateInfo floatInfo;
    floatInfo.name = "testFloat";
    floatInfo.defaultValue = "2.5";
    floatInfo.type = CVar_Float;
    CVar* testFloat = CVarCreate(floatInfo);
    VERIFY(2.5f == CVarReadFloat(testFloat));
    VERIFY(0 == CVarReadInt(testFloat));
    VERIFY(0 == CVarReadString(testFloat));

    CVarCreateInfo stringInfo;
    stringInfo.name = "testString";
    stringInfo.defaultValue = "gnyrf";
    stringInfo.type = CVar_String;
    CVar* testString = CVarCreate(stringInfo);
    VERIFY(0 == Util::String::StrCmp("gnyrf", CVarReadString(testString)));
    VERIFY(0 == CVarReadInt(testString));
    VERIFY(0 == CVarReadFloat(testString));

    CVar* intVar = CVarGet("testInt");
    VERIFY(intVar == testInt);
    CVar* floatVar = CVarGet("testFloat");
    VERIFY(floatVar == testFloat);
    CVar* strVar = CVarGet("testString");
    VERIFY(strVar == testString);

    CVarWriteInt(intVar, 5);
    VERIFY(5 == CVarReadInt(intVar));
    VERIFY(5 == CVarReadInt(testInt));

    CVarWriteFloat(floatVar, 5);
    VERIFY(5.0f == CVarReadFloat(floatVar));
    
    const char* newStr = "Testing, testing... Everything seems to be in order.";
    CVarWriteString(strVar, newStr);
    VERIFY(0 == Util::String::StrCmp(newStr, CVarReadString(strVar)));
}

}; 