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

void PassVec2(MonoObject* obj)
{
    reg_vec2 = Mono::Vector2::Convert(obj);
};

void PassVec3(MonoObject* obj)
{
    reg_vec3 = Mono::Vector3::Convert(obj);
};

void PassVec4(MonoObject* obj)
{
    reg_vec4 = Mono::Vector4::Convert(obj);
};

NEBULA_EXPORT void PassString(Util::String const& string)
{
    reg_string = string;
}

//------------------------------------------------------------------------------
/**
*/
void
SetupInternalCalls()
{
    mono_add_internal_call("NST.Tests/InternalCalls::TestPassVec2", &PassVec2);
    mono_add_internal_call("NST.Tests/InternalCalls::TestPassVec3", &PassVec3);
    mono_add_internal_call("NST.Tests/InternalCalls::TestPassVec4", &PassVec4);
}

//------------------------------------------------------------------------------
/**
*/
void
NSharpTest::Run()
{
    Ptr<MonoServer> monoServer = MonoServer::Create();
    monoServer->SetDebuggingEnabled(true);
    monoServer->WaitForDebuggerToConnect(false);
    monoServer->Open();

    Scripting::MonoAssemblyId assemblyId = monoServer->Load("bin:NSharpTests.dll");
    bool assemblyLoaded = assemblyId != Scripting::MonoAssemblyId::Invalid();
    VERIFY(assemblyLoaded);
    if (!assemblyLoaded)
    {
        n_warning("Failed to load assembly. Cannot proceed with test!\n");
        return;
    }

    VERIFY(0 == monoServer->Exec(assemblyId, "NST.AppEntry::Main()"));

    SetupInternalCalls();
    VERIFY(0 == monoServer->Exec(assemblyId, "NST.Tests/InternalCalls::RunTests()"));

    Math::vec2 const v2 = { 1.0f, 2.0f };
    VERIFY(reg_vec2 == v2);

    Math::vec3 const v3 = { 1.0f, 2.0f, 3.0f };
    VERIFY(reg_vec3 == v3);

    Math::vec4 const v4 = { 1.0f, 2.0f, 3.0f, 4.0f };
    VERIFY(reg_vec4 == v4);

    VERIFY(0 == monoServer->Exec(assemblyId, "NST.Tests/DLLImportCalls::RunTests()"));

    VERIFY(reg_string == "This is a C# string!\n");
    VERIFY(reg_string.Length() == 21); // make sure we're not just reading from the heap pointer...
    n_printf(reg_string.AsCharPtr());
}

}