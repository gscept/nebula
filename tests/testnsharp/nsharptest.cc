//------------------------------------------------------------------------------
//  nsharptest.cc
//  (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "nsharptest.h"
#include "nsharp/monoserver.h"
#include "nsharp/conversion/vector2.h"
#include "nsharp/conversion/vector3.h"
#include "nsharp/conversion/vector4.h"

using namespace Scripting;

namespace Test
{
__ImplementClass(Test::NSharpTest, 'nshT', Test::TestCase);

static Math::vec2 reg_vec2;
static Math::vec3 reg_vec3;
static Math::vec4 reg_vec4;
static Util::String reg_string;
static bool testArrayOfIntResult = false;
static bool testArrayOfVec3Result = false;
static bool testArrayOfVec4Result = false;

typedef struct _Vector2
{
    float x;
    float y;
} Vector2;

static std::function<void(bool, const char*, int)> verifyManagedCallback;

NEBULA_EXPORT void
VerifyManaged(bool success, const char* filePath, int lineNumber)
{
    verifyManagedCallback(success, filePath, lineNumber);
};

NEBULA_EXPORT void
PassVec2(Math::vec2 const& obj)
{
    reg_vec2 = obj;
};

NEBULA_EXPORT void
PassVec3(Math::vec3 const& obj)
{
    reg_vec3 = obj;
};

NEBULA_EXPORT void
PassVec4(Math::vec4 const& obj)
{
    reg_vec4 = obj;
};

NEBULA_EXPORT void
PassString(Util::String const& string)
{
    reg_string = string;
}

NEBULA_EXPORT void
TestArrayOfInt(int const* const arr, int size)
{
    testArrayOfIntResult = size == 10;
    int const values[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    for (IndexT i = 0; i < size; i++)
    {
        if (arr[i] != values[i])
        {
            testArrayOfIntResult = false;
        }
    }
}

NEBULA_EXPORT void
TestArrayOfVec3(Math::vec3* arr, int size)
{
    testArrayOfVec3Result = size == 10;
    Math::vec3 const values[10] = {
        {1, 2, 3},
        {1, 2, 3},
        {1, 2, 3},
        {1, 2, 3},
        {1, 2, 3},
        {1, 2, 3},
        {1, 2, 3},
        {1, 2, 3},
        {1, 2, 3},
        {1, 2, 3},
    };

    for (IndexT i = 0; i < 10; i++)
    {
        if (arr[i] != values[i])
        {
            testArrayOfVec3Result = false;
        }
    }
}

NEBULA_EXPORT void
TestArrayOfVec4(Math::vec4* arr, int size)
{
    testArrayOfVec4Result = size == 10;
    Math::vec4 const values[10] = {
        {1, 2, 3, 4},
        {1, 2, 3, 4},
        {1, 2, 3, 4},
        {1, 2, 3, 4},
        {1, 2, 3, 4},
        {1, 2, 3, 4},
        {1, 2, 3, 4},
        {1, 2, 3, 4},
        {1, 2, 3, 4},
        {1, 2, 3, 4},
    };

    for (IndexT i = 0; i < 10; i++)
    {
        if (arr[i] != values[i])
        {
            testArrayOfVec4Result = false;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NSharpTest::Run()
{
    verifyManagedCallback = [this](bool success, const char* filePath, int lineNumber)
    { this->Verify(success, "<Managed Function Test>", filePath, lineNumber); };

    Ptr<MonoServer> monoServer = MonoServer::Create();
    monoServer->SetDebuggingEnabled(true);
    monoServer->WaitForDebuggerToConnect(false);
    monoServer->Open();

    Scripting::NSharpAssemblyId assemblyId = monoServer->LoadAssembly("bin:NSharpTests.dll");
    bool assemblyLoaded = assemblyId != Scripting::NSharpAssemblyId::Invalid();
    VERIFY(assemblyLoaded);
    if (!assemblyLoaded)
    {
        n_warning("Failed to load assembly. Cannot proceed with test!\n");
        return;
    }

    VERIFY(0 == monoServer->ExecUnmanagedCall(assemblyId, "NST.AppEntry::Main()"));

    VERIFY(0 == monoServer->ExecUnmanagedCall(assemblyId, "NST.Tests+VariablePassing::RunTests()"));

    Math::vec2 const v2 = {1.0f, 2.0f};
    VERIFY(reg_vec2 == v2);

    Math::vec3 const v3 = {1.0f, 2.0f, 3.0f};
    VERIFY(reg_vec3 == v3);

    Math::vec4 const v4 = {1.0f, 2.0f, 3.0f, 4.0f};
    VERIFY(reg_vec4 == v4);

    VERIFY(0 == monoServer->ExecUnmanagedCall(assemblyId, "NST.Tests+DLLImportCalls::RunTests()"));

    VERIFY(reg_string == "This is a C# string!\n");
    VERIFY(reg_string.Length() == 21); // make sure we're not just reading from the heap pointer...
    n_printf(reg_string.AsCharPtr());

    VERIFY(testArrayOfIntResult);
    VERIFY(testArrayOfVec3Result);
    VERIFY(testArrayOfVec4Result);
}

} // namespace Test