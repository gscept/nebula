//------------------------------------------------------------------------------
//  nsharptest.cc
//  (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "nsharptest.h"
#include "nsharp/nsconfig.h"
#include "nsharp/propertyextension.h"
#include "timing/timer.h"
#include "profiling/profiling.h"
#include "game/gameserver.h"
#include "game/api.h"
#include "game/component.h"
#include "game/world.h"
#include "game/timesource.h"
#include "memdb/attributeregistry.h"
#include "components/nsharpcomponents.h"

#include "nsharp/api/input.h"
#include "input/inputserver.h"
#include "input/inputevent.h"
#include "input/inputhandler.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/key.h"
#include "input/mousebutton.h"

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

// Entities created by the test runner for the managed side to discover
static uint64_t nativeEntityForManaged = 0;
static uint64_t componentEntityForManaged = 0;

NEBULA_EXPORT uint64_t
GetNativeEntityIdForManaged()
{
    return nativeEntityForManaged;
}

NEBULA_EXPORT uint64_t
GetComponentEntityIdForManaged()
{
    return componentEntityForManaged;
}

// The managed side mirrors a world it did not create, so the test runner creates and destroys one
// and hands the id to the managed test. This is the editor case: worlds are authored in C++.
static Game::WorldHash testWorldHash;

NEBULA_EXPORT uint32_t
CreateTestWorld()
{
    testWorldHash = Game::WorldHash{9999};
    Game::World* world = Game::GameServer::Instance()->CreateWorld(testWorldHash);
    return uint32_t(world->GetWorldId());
}

NEBULA_EXPORT void
DestroyTestWorld()
{
    Game::GameServer::Instance()->DestroyWorld(testWorldHash);
}

// The mode a world runs in is what decides whether a managed property runs, so the test runner flips it
// here and the managed side reads it through the api
NEBULA_EXPORT void
SetTestWorldSimulating(uint32_t worldId, bool simulating)
{
    Game::World* world = Game::GetWorld(worldId);
    if (world == nullptr)
        return;

    world->mode = simulating ? Game::PlayMode : Game::EditorMode;
}

// The managed time source test needs a source that advances in fixed steps, and the test runner is the side
// that can drive the rate explicitly. The hash is the one the managed test asks the mirror for
static const uint32_t testTimeSourceHash = uint32_t('TST1');

NEBULA_EXPORT uint32_t
CreateTestTimeSource(double fixedFrameTime, int maxStepsPerFrame)
{
    Game::TimeSourceCreateInfo info;
    info.hash = testTimeSourceHash;
    info.fixedFrameTime = fixedFrameTime;
    info.maxStepsPerFrame = maxStepsPerFrame;
    Game::Time::CreateTimeSource(info);
    return testTimeSourceHash;
}

NEBULA_EXPORT void
AdvanceTestTimeSource(double delta)
{
    if (!Game::Time::HasTimeSource(testTimeSourceHash))
        return;

    Game::Time::AdvanceTimeSource(Game::Time::GetTimeSource(testTimeSourceHash), delta);
}

// The input mirror test
static Ptr<Input::InputServer> testInputServer;

NEBULA_EXPORT void
OpenTestInputServer()
{
    testInputServer = Input::InputServer::Create();
    testInputServer->Open();
}

NEBULA_EXPORT void
CloseTestInputServer()
{
    if (!testInputServer.isvalid())
        return;

    testInputServer->Close();
    testInputServer = nullptr;
}

// The layout contract has nothing but the field order, so both sides state the size and the test compares
// the two statements
NEBULA_EXPORT int
InputTestSnapshotSize()
{
    return (int)sizeof(Scripting::Api::InputSnapshot);
}

NEBULA_EXPORT int
InputTestKeyCount()
{
    return (int)Input::Key::NumKeyCodes;
}

NEBULA_EXPORT int
InputTestMouseButtonCount()
{
    return (int)Input::MouseButton::NumMouseButtons;
}

// A key code as the engine's own dictionary reads it. The managed enum is written out by hand, so this is
// where the test checks that the two agree
NEBULA_EXPORT int
InputTestKeyCode(const char* name)
{
    return (int)Input::Key::FromString(Util::String(name));
}

NEBULA_EXPORT int
InputTestMouseButtonCode(const char* name)
{
    return (int)Input::MouseButton::FromString(Util::String(name));
}

NEBULA_EXPORT void
InputTestBeginFrame()
{
    testInputServer->BeginFrame();
}

NEBULA_EXPORT void
InputTestEndFrame()
{
    testInputServer->EndFrame();
}

NEBULA_EXPORT void
InputTestPutKey(int code, bool down)
{
    Input::InputEvent e;
    e.SetType(down ? Input::InputEvent::KeyDown : Input::InputEvent::KeyUp);
    e.SetKey((Input::Key::Code)code);
    testInputServer->PutEvent(e);
}

NEBULA_EXPORT void
InputTestPutMouseButton(int button, bool down)
{
    Input::InputEvent e;
    e.SetType(down ? Input::InputEvent::MouseButtonDown : Input::InputEvent::MouseButtonUp);
    e.SetMouseButton((Input::MouseButton::Code)button);

    // The window sends the position it has with a button event, and the handler updates its position from
    // that, so the test carries the position the frame already has
    e.SetAbsMousePos(testInputServer->GetDefaultMouse()->GetPixelPosition());
    e.SetNormMousePos(testInputServer->GetDefaultMouse()->GetScreenPosition());

    testInputServer->PutEvent(e);
}

NEBULA_EXPORT void
InputTestPutMouseMove(float x, float y)
{
    Input::InputEvent e;
    e.SetType(Input::InputEvent::MouseMove);
    e.SetAbsMousePos(Math::vec2(x, y));
    e.SetNormMousePos(Math::vec2(x, y));
    testInputServer->PutEvent(e);
}

// The window is the one that re-bases the handler after a warp, and the test has no window, so the handler is
// re-based here with the same values the window would have computed
NEBULA_EXPORT void
InputTestWarpMouse(float x, float y)
{
    testInputServer->GetDefaultMouse()->WarpPosition(Math::vec2(x, y), Math::vec2(x, y));
}

NEBULA_EXPORT void
InputTestPutMouseWheel(int clicks)
{
    Input::InputEvent e;
    e.SetType(clicks >= 0 ? Input::InputEvent::MouseWheelForward : Input::InputEvent::MouseWheelBackward);

    // A window sends the position it has with a wheel event, and the handler takes its position from any
    // event it gets, so the test carries the position the frame already has rather than an empty one
    e.SetAbsMousePos(testInputServer->GetDefaultMouse()->GetPixelPosition());
    e.SetNormMousePos(testInputServer->GetDefaultMouse()->GetScreenPosition());

    testInputServer->PutEvent(e);
}

NEBULA_EXPORT void
InputTestPutFocus(bool focus)
{
    Input::InputEvent e;
    e.SetType(focus ? Input::InputEvent::AppObtainFocus : Input::InputEvent::AppLoseFocus);
    testInputServer->PutEvent(e);
}

// The character the keyboard produced during the frame. Text is the one part of the keyboard state that
// cannot come through the mirror, because a DllImport that returns a string makes the marshaler free the
// pointer it was given, so the managed side reads it through a buffer it owns
NEBULA_EXPORT void
InputTestPutChar(int chr)
{
    Input::InputEvent e;
    e.SetType(Input::InputEvent::Character);
    e.SetChar((Input::Char)chr);
    testInputServer->PutEvent(e);
}

// Capture is a handler state rather than an event, so this goes through the server directly. The handler that
// holds the capture is the one the events go to, which is the state the mirror reports
NEBULA_EXPORT void
InputTestCaptureMouse(bool capture)
{
    Ptr<Input::InputHandler> mouse = testInputServer->GetDefaultMouse().upcast<Input::InputHandler>();

    if (capture)
        testInputServer->ObtainMouseCapture(mouse);
    else
        testInputServer->ReleaseMouseCapture(mouse);
}

NEBULA_EXPORT void
InputTestCaptureKeyboard(bool capture)
{
    Ptr<Input::InputHandler> keyboard = testInputServer->GetDefaultKeyboard().upcast<Input::InputHandler>();

    if (capture)
        testInputServer->ObtainKeyboardCapture(keyboard);
    else
        testInputServer->ReleaseKeyboardCapture(keyboard);
}

// How many times the managed property's OnBeginFrame and OnMessage run. A property that belongs
// to a retired plugin assembly must not keep being called after a reload, so the test counts the
// calls against the per-instance state the property itself keeps.
static int propertyFrameCalls = 0;
static int propertyMessageCalls = 0;
static int loadAttempts = 0;

NEBULA_EXPORT void
RecordLoadAttempt()
{
    loadAttempts++;
}

NEBULA_EXPORT int
GetLoadAttempts()
{
    return loadAttempts;
}

NEBULA_EXPORT void
RecordPropertyFrameCall()
{
    propertyFrameCalls++;
}

NEBULA_EXPORT void
RecordPropertyMessageCall()
{
    propertyMessageCalls++;
}

NEBULA_EXPORT int
GetPropertyFrameCalls()
{
    return propertyFrameCalls;
}

NEBULA_EXPORT int
GetPropertyMessageCalls()
{
    return propertyMessageCalls;
}

// The level format's contribution for managed properties, driven from the test runner. The managed
// side cannot call the extension itself, so the capture and the apply go through here, and the capture
// is checked here as well, because this is where the buffer the managed side writes into is. The
// results are recorded and verified at the end of the run, since the exports are not test methods.
static uint64_t propertySourceEntity = 0;
static uint64_t propertyTargetEntity = 0;

// An entity whose instance is taken back out while its id is still held. This is what an editor command
// holds while it is on the undo stack, and it is why a valid entity is not necessarily one a component
// access can read
static uint64_t instancelessEntity = 0;
static Util::String capturedExtensionJson;
static bool capturedJsonLengthOk = false;
static bool capturedJsonTextOk = false;

// The mutation contract. The recorder is the same hook the editor installs; here the test installs its own,
// because the managed write path does not write anything itself. What the recorder does is what a command does:
// apply the text it is given, exact, which is what makes an empty text the removal
static Util::String recordedPath;
static Util::String recordedOldText;
static Util::String recordedNewText;
static bool recordedChangeApplied = false;

void
TestPropertyChange(uint64_t entityId, const char* path, const char* oldText, const char* newText)
{
    recordedPath.SetCharPtr(path);
    recordedOldText.SetCharPtr(oldText);
    recordedNewText.SetCharPtr(newText);

    Scripting::PropertyLevelExtension::Apply(Game::Entity(entityId), newText);
    recordedChangeApplied = true;
}

// The path the recorder was told about. The editor builds its override marker from this, and it is the only
// place the editor learns what a key inside the text means
NEBULA_EXPORT bool
RecordedPathMatches(const char* expected)
{
    return recordedPath == Util::String(expected);
}

NEBULA_EXPORT uint64_t
GetPropertySourceEntityId()
{
    return propertySourceEntity;
}

NEBULA_EXPORT uint64_t
GetPropertyTargetEntityId()
{
    return propertyTargetEntity;
}

NEBULA_EXPORT uint64_t
GetInstancelessEntityId()
{
    return instancelessEntity;
}

NEBULA_EXPORT void
CaptureEntityJson(uint64_t entityId)
{
    capturedExtensionJson.Clear();
    Scripting::PropertyLevelExtension::Capture(Game::Entity(entityId), capturedExtensionJson);

    // The property the managed side writes has a name longer than the buffer the capture starts with,
    // so this is the path where the managed side answers with the length it needs and the call is made
    // again with a bigger buffer
    capturedJsonLengthOk = capturedExtensionJson.Length() > 1024;

    Util::String longName;
    for (int i = 0; i < 1200; i++)
        longName.Append("x");

    const Util::String expected =
        "{\"FormatTestProperty\":{\"id\":7,\"name\":\"" + longName + "\",\"position\":[1.5,-2,3],\"scale\":2.5}}";
    capturedJsonTextOk = capturedExtensionJson == expected;
}

NEBULA_EXPORT void
ApplyPropertyJson(uint64_t entityId)
{
    Scripting::PropertyLevelExtension::Apply(Game::Entity(entityId), capturedExtensionJson.AsCharPtr());
}

NEBULA_EXPORT void
ApplyEntityText(uint64_t entityId, const char* text)
{
    Scripting::PropertyLevelExtension::Apply(Game::Entity(entityId), text);
}

//------------------------------------------------------------------------------
/**
    The text a revert composes, read through the extension's hooks rather than through the managed methods
    directly. This is the path the editor takes, and it is the only way a test can see that the entry points the
    extension cached actually resolved.
*/
NEBULA_EXPORT bool
ResolveThroughExtension(const char* baseText, const char* keys, const char* expected)
{
    Util::Array<Util::String> parsed;
    const char* pos = keys;
    while (*pos != 0)
    {
        const char* stop = pos;
        while (*stop != 0 && *stop != '\n')
            stop++;

        Util::String name;
        name.Set(pos, (SizeT)(stop - pos));
        if (!name.IsEmpty())
            parsed.Append(name);

        pos = stop + 1;
    }

    Util::String out;
    Scripting::PropertyLevelExtension::Resolve(baseText, parsed, out);
    return out == Util::String(expected);
}

//------------------------------------------------------------------------------
/**
    The entity references a text names, read through the extension's hook rather than through the
    managed method directly, which is the path the document takes. The answer comes back as the same
    block of "path\tguid" lines the extension hands over, because the container C++ holds them in is
    not something the managed side can read back.
*/
NEBULA_EXPORT bool
TextRefsMatch(const char* text, const char* expectedPairs)
{
    Util::Array<Game::TextRef> named;
    Scripting::PropertyLevelExtension::TextRefs(text, named);

    Util::String pairs;
    for (IndexT i = 0; i < named.Size(); i++)
    {
        if (!pairs.IsEmpty())
            pairs += "\n";

        pairs += named[i].path;
        pairs += "\t";

        // A reference with no target is a line with nothing after the tab, which is what the writer
        // gives an extension for a target the file has no record for
        if (named[i].guid.IsValid())
            pairs += named[i].guid.AsString();
    }

    return pairs == Util::String(expectedPairs);
}

//------------------------------------------------------------------------------
/**
    The text with the named paths' references put as the given guids, through the extension's hook.
    This is the path the document takes when it writes a text it cannot read.
*/
NEBULA_EXPORT bool
RemappedTextMatches(const char* text, const char* pairs, const char* expected)
{
    Util::String out;
    Scripting::PropertyLevelExtension::Remap(text, pairs, out);
    return out == Util::String(expected);
}

//------------------------------------------------------------------------------
/**
    The override marker. The marker is the editor's own and this side cannot read it, so a row asks for one key at
    a time through a callback. The scene's marker is not reachable from a test, so the test installs its own
    answer here and records which key the row asked about.
*/
static Util::String recordedAuthoredPath;
static bool recordedAuthoredAnswer = false;

bool
TestIsAuthored(uint64_t entityId, const char* path)
{
    recordedAuthoredPath.SetCharPtr(path);
    return recordedAuthoredAnswer;
}

NEBULA_EXPORT void
SetAuthoredAnswer(const char* path, bool answer)
{
    recordedAuthoredPath.Clear();
    recordedAuthoredAnswer = answer;
}

NEBULA_EXPORT bool
AuthoredPathMatches(const char* expected)
{
    return recordedAuthoredPath == Util::String(expected);
}

// The managed side cannot read what the native side recorded, and a string returned from C++ is one the
// marshaler would free, so the test hands the text it expects here and the comparison is done on this side
NEBULA_EXPORT bool
CapturedJsonMatches(const char* expected)
{
    return capturedExtensionJson == Util::String(expected);
}

NEBULA_EXPORT bool
RecordedChangeMatches(const char* path, const char* oldText, const char* newText)
{
    return recordedPath == Util::String(path) &&
        recordedOldText == Util::String(oldText) &&
        recordedNewText == Util::String(newText) &&
        recordedChangeApplied;
}

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
PassStringUtf8(const char* string, int size)
{
    reg_string.Set(string, size);
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

// The layout the engine has for a component, which is what a managed mirror has to match. The
// generator builds the managed struct from its own size table, so the managed test compares that
// table against these, and these come from the compiler rather than from the table.

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT uint32_t
ComponentTypeSizeForManaged(uint32_t componentId)
{
    return (uint32_t)MemDb::AttributeRegistry::TypeSize(componentId);
}

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT uint32_t
ComponentNumFieldsForManaged(uint32_t componentId)
{
    Game::ComponentInterface* cInterface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(componentId));
    if (cInterface == nullptr)
        return 0;

    return (uint32_t)cInterface->GetNumFields();
}

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT uint32_t
ComponentFieldOffsetForManaged(uint32_t componentId, uint32_t fieldIndex)
{
    Game::ComponentInterface* cInterface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(componentId));
    if (cInterface == nullptr || fieldIndex >= (uint32_t)cInterface->GetNumFields())
        return 0;

    return (uint32_t)cInterface->GetFieldByteOffsets()[fieldIndex];
}

//------------------------------------------------------------------------------
/**
    The name is copied into a buffer the caller owns. A DllImport that returns a string makes the
    marshaler free the pointer it was given, and that pointer is the engine's string.
*/
NEBULA_EXPORT bool
ComponentFieldNameForManaged(uint32_t componentId, uint32_t fieldIndex, char* outBuffer, uint32_t bufferSize)
{
    if (bufferSize == 0)
        return false;

    Game::ComponentInterface* cInterface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(componentId));
    if (cInterface == nullptr || fieldIndex >= (uint32_t)cInterface->GetNumFields())
        return false;

    const char* name = cInterface->GetFieldNames()[fieldIndex];
    uint32_t i = 0;
    while (i + 1 < bufferSize && name[i] != 0)
    {
        outBuffer[i] = name[i];
        i++;
    }
    outBuffer[i] = 0;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
NSharpTest::Run()
{
#if NEBULA_ENABLE_PROFILING
    Profiling::ProfilingRegisterThread();
#endif

    verifyManagedCallback = [this](bool success, const char* filePath, int lineNumber)
    { this->Verify(success, "<Managed Function Test>", filePath, lineNumber); };

    // The recorder the managed write path reports to. In the editor this is the function that builds the
    // command, and the command is the only thing that changes a property, so the test installs its own and
    // applies the change the way a command would
    Scripting::PropertyLevelExtension::SetChangeCallback(&TestPropertyChange);

    // The override marker. The marker is the scene's own and the managed side cannot read it, so a row asks
    // through a callback. The scene is not here, so the test answers for it
    Scripting::PropertyLevelExtension::SetAuthoredCallback(&TestIsAuthored);

    // The input server the managed mirror reads. Opening it is what creates the default keyboard and mouse
    // handlers the mirror reports, and there is no window here to pump events into them
    OpenTestInputServer();

    // Entities created by the test runner for the managed side to discover through the entity
    // callback. This is what a plugin needs in the editor: entities are authored by C++, not by
    // the script, so the managed World has to mirror the native one.
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    VERIFY(world != nullptr);

    nativeEntityForManaged = uint64_t(world->CreateEntity(true));

    // Components can only be added from native code, so prepare an entity with the components
    // whose managed type mapping the test checks
    Game::Entity componentEntity = world->CreateEntity(true);
    world->AddComponent<Test::TestFlags>(componentEntity);
    world->AddComponent<Test::TestAtom>(componentEntity);
    world->ExecuteAddComponentCommands();
    componentEntityForManaged = uint64_t(componentEntity);

    // Two entities for the level format test: the managed side puts a property on the first, the
    // format writes it as text, and that text is applied to the second, which has nothing on it
    propertySourceEntity = uint64_t(world->CreateEntity(true));
    propertyTargetEntity = uint64_t(world->CreateEntity(true));

    // An entity with an instance, and then the instance taken back out again while the id is kept. This
    // is the state an editor command is in while it is on the undo stack
    Game::Entity instanceless = world->CreateEntity(true);
    world->DeallocateInstance(instanceless);
    instancelessEntity = uint64_t(instanceless);

    StepFrame();

    Math::vec2 const v2 = {1.0f, 2.0f};
    VERIFY(reg_vec2 == v2);

    Math::vec3 const v3 = {1.0f, 2.0f, 3.0f};
    VERIFY(reg_vec3 == v3);

    Math::vec4 const v4 = {1.0f, 2.0f, 3.0f, 4.0f};
    VERIFY(reg_vec4 == v4);

    VERIFY(reg_string == "This is a C# string!\n");
    VERIFY(reg_string.Length() == 21); // make sure we're not just reading from the heap pointer...
    n_printf(reg_string.AsCharPtr());

    VERIFY(testArrayOfIntResult);
    VERIFY(testArrayOfVec3Result);
    VERIFY(testArrayOfVec4Result);

    // One frame per sync phase, then the reloads, the failed reload and the verification after them
    for (IndexT i = 0; i < 13; i++)
        StepFrame();
    StepFrame();
    StepFrame();
    StepFrame();  // the property level format round trip
    StepFrame();  // the mutation contract
    StepFrame();  // the entity that is an id without an instance
    StepFrame();  // the sim world test, one frame per phase
    StepFrame();
    StepFrame();
    StepFrame();
    StepFrame();
    StepFrame();  // the time source mirror, one frame per phase
    StepFrame();
    StepFrame();
    StepFrame();  // the merge, and the entity reference inside the extension text
    StepFrame();
    StepFrame();  // the text a revert composes from

    StepFrame();  // the input mirror, one frame per phase
    StepFrame();
    StepFrame();
    StepFrame();
    StepFrame();
    StepFrame();
    StepFrame();  // the text the keyboard produced, and the server is closed after it

    StepFrame();  // the layout the generator assumed for every component, against the engine's own

    // The activation gate, one frame per phase
    for (IndexT i = 0; i < 7; i++)
        StepFrame();

    // The action queue, one frame per phase
    for (IndexT i = 0; i < 9; i++)
        StepFrame();

    // The activation order guarantee, one frame per phase
    StepFrame();
    StepFrame();

    // The events a property gets without saying which ones, one frame per phase
    StepFrame();
    StepFrame();
    StepFrame();

    VERIFY(capturedJsonLengthOk);
    VERIFY(capturedJsonTextOk);

    Scripting::PropertyLevelExtension::SetChangeCallback(nullptr);
    Scripting::PropertyLevelExtension::SetAuthoredCallback(nullptr);

    CloseTestInputServer();
}

} // namespace Test
