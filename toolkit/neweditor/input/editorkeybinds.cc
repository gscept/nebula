//------------------------------------------------------------------------------
//  @file editorkeybinds.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/input/editorkeybinds.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/editorlog.h"
#include "io/ioserver.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"

namespace Editor
{

__ImplementClass(Editor::EditorKeybinds, 'EKBD', Core::RefCounted);

// The key names the file and the console command use. They are lowercase because a file a user
// edits is easier to
// read and easier to type, and the parser reads them case insensitively. A key that is not in this
// table is
// written as its number, so a file that holds one is still readable, just not nice.
struct KeyName
{
    Input::Key::Code code;
    const char* name;
};

static const KeyName KeyNames[] =
{
    { Input::Key::A, "a" },
    { Input::Key::B, "b" },
    { Input::Key::C, "c" },
    { Input::Key::D, "d" },
    { Input::Key::E, "e" },
    { Input::Key::F, "f" },
    { Input::Key::G, "g" },
    { Input::Key::H, "h" },
    { Input::Key::I, "i" },
    { Input::Key::J, "j" },
    { Input::Key::K, "k" },
    { Input::Key::L, "l" },
    { Input::Key::M, "m" },
    { Input::Key::N, "n" },
    { Input::Key::O, "o" },
    { Input::Key::P, "p" },
    { Input::Key::Q, "q" },
    { Input::Key::R, "r" },
    { Input::Key::S, "s" },
    { Input::Key::T, "t" },
    { Input::Key::U, "u" },
    { Input::Key::V, "v" },
    { Input::Key::W, "w" },
    { Input::Key::X, "x" },
    { Input::Key::Y, "y" },
    { Input::Key::Z, "z" },

    { Input::Key::Key0, "0" },
    { Input::Key::Key1, "1" },
    { Input::Key::Key2, "2" },
    { Input::Key::Key3, "3" },
    { Input::Key::Key4, "4" },
    { Input::Key::Key5, "5" },
    { Input::Key::Key6, "6" },
    { Input::Key::Key7, "7" },
    { Input::Key::Key8, "8" },
    { Input::Key::Key9, "9" },

    { Input::Key::F1, "f1" },
    { Input::Key::F2, "f2" },
    { Input::Key::F3, "f3" },
    { Input::Key::F4, "f4" },
    { Input::Key::F5, "f5" },
    { Input::Key::F6, "f6" },
    { Input::Key::F7, "f7" },
    { Input::Key::F8, "f8" },
    { Input::Key::F9, "f9" },
    { Input::Key::F10, "f10" },
    { Input::Key::F11, "f11" },
    { Input::Key::F12, "f12" },

    { Input::Key::Space, "space" },
    { Input::Key::Return, "enter" },
    { Input::Key::Tab, "tab" },
    { Input::Key::Escape, "escape" },
    { Input::Key::Back, "backspace" },
    { Input::Key::Delete, "delete" },
    { Input::Key::Insert, "insert" },
    { Input::Key::Home, "home" },
    { Input::Key::End, "end" },
    { Input::Key::PageUp, "pageup" },
    { Input::Key::PageDown, "pagedown" },
    { Input::Key::Up, "up" },
    { Input::Key::Down, "down" },
    { Input::Key::Left, "left" },
    { Input::Key::Right, "right" },

    { Input::Key::Minus, "minus" },
    { Input::Key::Equality, "equal" },
    { Input::Key::Comma, "comma" },
    { Input::Key::Period, "period" },
    { Input::Key::Slash, "slash" },
    { Input::Key::Semicolon, "semicolon" },
    { Input::Key::Quote, "quote" },
    { Input::Key::BackSlash, "backslash" },
    { Input::Key::LeftBracket, "bracketleft" },
    { Input::Key::RightBracket, "bracketright" },
    { Input::Key::Tilde, "grave" },
};

static const IndexT NumKeyNames = (IndexT)sizeof(KeyNames) / sizeof(KeyNames[0]);

// A combo token that is not a name is read as the key number, which is what lets a key the table
// does not name
// round trip through the file. The CRT number parse is not available, so this reads the digits
// itself.
static bool
KeyNumberFromToken(const Util::String& token, Input::Key::Code& code)
{
    if (token.IsEmpty())
        return false;

    IndexT i = 0;
    bool negative = false;
    if (token[0] == '-')
    {
        negative = true;
        i = 1;
    }
    if (i >= token.Length())
        return false;

    int32_t value = 0;
    for (; i < token.Length(); i++)
    {
        const char c = token[i];
        if (c < '0' || c > '9')
            return false;
        value = value * 10 + (c - '0');
    }

    code = (Input::Key::Code)(negative ? -value : value);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
EditorKeybinds::EditorKeybinds() :
    core(nullptr),
    path("bin:keybinds.json"),
    captureMods(Input::Modifier::NoModifier)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
EditorKeybinds::~EditorKeybinds()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
EditorKeybinds::Setup(Shell* core)
{
    this->core = core;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorKeybinds::Discard()
{
    this->keybinds.Clear();
    this->capturing.Clear();
    this->captureMods = Input::Modifier::NoModifier;
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorKeybinds::SetPath(const Util::String& path)
{
    this->path = path;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
EditorKeybinds::GetPath() const
{
    return this->path;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorKeybinds::SetCapture(const Util::String& action)
{
    this->capturing = action;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorKeybinds::ClearCapture()
{
    this->capturing.Clear();
    this->captureMods = Input::Modifier::NoModifier;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorKeybinds::IsCapturing() const
{
    return !this->capturing.IsEmpty();
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
EditorKeybinds::GetCapture() const
{
    return this->capturing;
}

//------------------------------------------------------------------------------
/**
*/
Input::Modifier::Code
EditorKeybinds::GetCaptureMods() const
{
    return this->captureMods;
}

//------------------------------------------------------------------------------
/**
    The action is what the file and the console command use, so it has to be the same name every
    time the same thing is registered. A combo that another action already has is a conflict rather
    than a redefinition: both actions stay in the registry, and the warning names the one that
    already had the combo, because two equal binds are settled by the order they are listed in.
*/
void
EditorKeybinds::Register(const Util::String& action, Input::Key::Code key, Input::Modifier::Code mods, const Util::String& command)
{
    n_assert(!action.IsEmpty());
    n_assert(key != Input::Key::InvalidKey);
    n_assert(!command.IsEmpty());

    const IndexT existing = this->FindIndex(action);
    if (existing != InvalidIndex)
    {
        this->keybinds[existing].defaultKey = key;
        this->keybinds[existing].defaultMods = mods;
        this->keybinds[existing].command = command;
        return;
    }

    EditorKeyBind keybind;
    keybind.action = action;
    keybind.key = key;
    keybind.mods = mods;
    keybind.defaultKey = key;
    keybind.defaultMods = mods;
    keybind.command = command;

    this->keybinds.Append(keybind);

    this->WarnConflict(this->keybinds.Size() - 1);
    this->Sort();
}

//------------------------------------------------------------------------------
/**
*/
IndexT
EditorKeybinds::FindIndex(const Util::String& action) const
{
    for (IndexT i = 0; i < this->keybinds.Size(); i++)
    {
        if (this->keybinds[i].action == action)
            return i;
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
const EditorKeyBind*
EditorKeybinds::Find(const Util::String& action) const
{
    const IndexT index = this->FindIndex(action);
    if (index == InvalidIndex)
        return nullptr;

    return &this->keybinds[index];
}

//------------------------------------------------------------------------------
/**
*/
void
EditorKeybinds::WarnConflict(IndexT index) const
{
    if (this->core == nullptr)
        return;

    const EditorKeyBind& keybind = this->keybinds[index];
    if (keybind.key == Input::Key::InvalidKey)
        return;

    // Only the entries before it are looked at, so a pair is reported once rather than from both
    // sides
    for (IndexT i = 0; i < index; i++)
    {
        if (this->keybinds[i].key == keybind.key && this->keybinds[i].mods == keybind.mods)
        {
            this->core->Log(LogMessageType::N_WARNING, Util::String("Keybind '") + keybind.action +
                "' uses " + ComboString(keybind.key, keybind.mods) + ", which " + this->keybinds[i].action +
                " already has");
            return;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorKeybinds::Bind(const Util::String& action, Input::Key::Code key, Input::Modifier::Code mods)
{
    const IndexT index = this->FindIndex(action);
    if (index == InvalidIndex)
        return false;

    this->keybinds[index].key = key;
    this->keybinds[index].mods = mods;

    this->WarnConflict(index);
    this->Sort();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorKeybinds::BindCombo(const Util::String& action, const Util::String& combo)
{
    Input::Key::Code key = Input::Key::InvalidKey;
    Input::Modifier::Code mods = Input::Modifier::NoModifier;
    if (!ComboFromString(combo, key, mods))
        return false;

    return this->Bind(action, key, mods);
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorKeybinds::Unbind(const Util::String& action)
{
    return this->Bind(action, Input::Key::InvalidKey, Input::Modifier::NoModifier);
}

//------------------------------------------------------------------------------
/**
*/
void
EditorKeybinds::Reset(const Util::String& action)
{
    if (action.IsEmpty())
    {
        for (IndexT i = 0; i < this->keybinds.Size(); i++)
        {
            this->keybinds[i].key = this->keybinds[i].defaultKey;
            this->keybinds[i].mods = this->keybinds[i].defaultMods;
        }
    }
    else
    {
        const IndexT index = this->FindIndex(action);
        if (index == InvalidIndex)
            return;

        this->keybinds[index].key = this->keybinds[index].defaultKey;
        this->keybinds[index].mods = this->keybinds[index].defaultMods;
    }

    this->Sort();
}

//------------------------------------------------------------------------------
/**
*/
Util::String
EditorKeybinds::ComboString(Input::Key::Code key, Input::Modifier::Code mods)
{
    if (key == Input::Key::InvalidKey)
        return Util::String();

    Util::String combo = Input::Modifier::ToString(mods);
    if (!combo.IsEmpty())
        combo.Append("+");

    for (IndexT i = 0; i < NumKeyNames; i++)
    {
        if (KeyNames[i].code == key)
        {
            combo.Append(KeyNames[i].name);
            return combo;
        }
    }

    // A key the table does not name. The number is what the parser reads back, so the combo still
    // round trips
    Util::String number;
    number.Format("%d", (int)key);
    combo.Append(number);
    return combo;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorKeybinds::ComboFromString(const Util::String& combo, Input::Key::Code& key, Input::Modifier::Code& mods)
{
    key = Input::Key::InvalidKey;
    mods = Input::Modifier::NoModifier;

    Util::String lower = combo;
    lower.ToLower();

    Util::Array<Util::String> tokens;
    lower.Tokenize("+", tokens);
    if (tokens.IsEmpty())
        return false;

    for (IndexT i = 0; i < tokens.Size(); i++)
    {
        const Util::String& token = tokens[i];

        const Input::Modifier::Code bit = Input::Modifier::FromString(token);
        if (bit != Input::Modifier::NoModifier)
        {
            mods |= bit;
            continue;
        }

        IndexT n = 0;
        for (; n < NumKeyNames; n++)
        {
            if (token == KeyNames[n].name)
            {
                key = KeyNames[n].code;
                break;
            }
        }

        if (n >= NumKeyNames && !KeyNumberFromToken(token, key))
            return false;
    }

    return key != Input::Key::InvalidKey;
}

//------------------------------------------------------------------------------
/**
    The file is read over the defaults, so this has to be called after everything has registered
    what it registers. An action the file has but nothing registered is a stale entry from an
    earlier version of the editor: it warns and is skipped rather than being added, because an
    action that nothing implements has nothing to run.
*/
bool
EditorKeybinds::Load()
{
    if (this->path.IsEmpty())
        return false;

    const IO::URI uri(this->path);
    if (!IO::FileExists(uri))
        return false;

    Ptr<IO::Stream> stream = IO::CreateStream(uri);
    if (!stream.isvalid())
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, Util::String("Could not open the keybind file ") + this->path);
        return false;
    }

    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, Util::String("Could not read the keybind file ") + this->path);
        return false;
    }

    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(stream);
    if (!reader->Open())
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, Util::String("Could not read the keybind file ") + this->path + " as json");
        if (stream->IsOpen())
            stream->Close();
        return false;
    }

    if (!reader->HasNode("bindings"))
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, "Keybind file has no bindings");
        reader->Close();
        return false;
    }

    reader->SetToNode("bindings");

    IndexT applied = 0;
    if (reader->SetToFirstChild())
    {
        do
        {
            if (!reader->IsObject())
            {
                if (this->core != nullptr)
                    this->core->Log(LogMessageType::N_WARNING, "Keybind file has a binding that is not an object");
                continue;
            }

            const Util::String action = reader->GetString("action");
            const Util::String combo = reader->GetString("combo");

            const IndexT index = this->FindIndex(action);
            if (index == InvalidIndex)
            {
                if (this->core != nullptr)
                    this->core->Log(LogMessageType::N_WARNING, Util::String("Keybind file has an action nothing registers: ") + action);
                continue;
            }

            if (combo.IsEmpty())
            {
                this->keybinds[index].key = Input::Key::InvalidKey;
                this->keybinds[index].mods = Input::Modifier::NoModifier;
                applied++;
                continue;
            }

            Input::Key::Code key = Input::Key::InvalidKey;
            Input::Modifier::Code mods = Input::Modifier::NoModifier;
            if (!ComboFromString(combo, key, mods))
            {
                if (this->core != nullptr)
                    this->core->Log(LogMessageType::N_WARNING, Util::String("Keybind file has a combo that cannot be read: ") + combo);
                continue;
            }

            this->keybinds[index].key = key;
            this->keybinds[index].mods = mods;
            applied++;
        }
        while (reader->SetToNextChild());
    }

    // Closing the reader closes the stream with it, so the stream is not closed on its own here
    reader->Close();

    this->Sort();

    // Two actions can end up on the same combo through the file, and the order the registry is kept
    // in is what
    // decides which one a key press lands on, so a collision is worth saying
    for (IndexT i = 0; i < this->keybinds.Size(); i++)
    {
        this->WarnConflict(i);
    }

    if (this->core != nullptr && applied > 0)
        this->core->Log(LogMessageType::N_SYSTEM, Util::String("Read ") + Util::String::Sprintf("%d", (int)applied) + " keybinds from " + this->path);

    return applied > 0;
}

//------------------------------------------------------------------------------
/**
    Every binding as it is now, not only the ones that changed, which is what makes the file the
    keymap rather than a list of exceptions to a keymap that is somewhere else.
*/
bool
EditorKeybinds::Save()
{
    if (this->path.IsEmpty())
        return false;

    const IO::URI uri(this->path);
    IO::EnsureDirectoriesForFile(uri);

    Ptr<IO::Stream> stream = IO::CreateStream(uri);
    if (!stream.isvalid())
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, Util::String("Could not open the keybind file ") + this->path + " for writing");
        return false;
    }

    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, Util::String("Could not write the keybind file ") + this->path);
        return false;
    }

    Ptr<IO::JsonWriter> writer = IO::JsonWriter::Create();
    writer->SetStream(stream);
    if (!writer->Open())
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, Util::String("Could not write the keybind file ") + this->path);
        if (stream->IsOpen())
            stream->Close();
        return false;
    }

    // The document is already an object when the writer opens, so the version and the bindings are
    // added to it
    // directly. A BeginObject here would have to be closed by an End, and an object with no name is
    // not what the
    // file should look like
    writer->Add(1, "version");

    writer->BeginArray("bindings");
    for (IndexT i = 0; i < this->keybinds.Size(); i++)
    {
        const EditorKeyBind& keybind = this->keybinds[i];

        writer->BeginObject();
        writer->Add(keybind.action, "action");
        writer->Add(ComboString(keybind.key, keybind.mods), "combo");
        writer->Add(keybind.command, "command");
        writer->End();
    }
    writer->End();

    // The document is serialized into the stream when the writer closes, and the writer closes the
    // stream with it
    writer->Close();

    if (this->core != nullptr)
        this->core->Log(LogMessageType::N_SYSTEM, Util::String("Wrote ") + Util::String::Sprintf("%d", (int)this->keybinds.Size()) + " keybinds to " + this->path);

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorKeybinds::ActionOrder(const EditorKeyBind& lhs, const EditorKeyBind& rhs)
{
    return lhs.action < rhs.action;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorKeybinds::Sort()
{
    this->keybinds.SortWithFunc(&EditorKeybinds::ActionOrder);
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorKeybinds::Process(Input::Key::Code key, bool isPress, Input::Modifier::Code mods)
{
    if (this->core == nullptr)
        return false;

    // While a row is listening, the key that comes is recorded rather than run. A modifier is only
    // half a combo,
    // so a press or a release of one records what is held and waits for the key that goes with it,
    // and Escape is
    // the key that stops listening rather than the key that binds. Both are read before the press
    // check below,
    // because a release is what takes a modifier back off when the user lets go of it before the
    // key comes
    if (!this->capturing.IsEmpty())
    {
        if (key == Input::Key::Escape)
        {
            this->ClearCapture();
            return true;
        }

        if (Input::Modifier::IsModifier(key))
        {
            this->captureMods = mods;
            return true;
        }
    }

    if (!isPress)
        return false;

    if (this->core->IsCameraCapturing())
        return true;

    if (!this->capturing.IsEmpty())
    {
        // The key the modal is waiting for is recorded rather than run, and written the same way a
        // bind from the
        // console is written. The action is cleared either way, because a row that stays listening
        // after its key
        // is a row that binds the next key the user meant for something else
        if (this->Bind(this->capturing, key, mods))
        {
            this->core->Log(LogMessageType::N_SYSTEM,
                Util::String("Bound ") + this->capturing + " to " + ComboString(key, mods));
            this->Save();
        }
        else
        {
            this->core->Log(LogMessageType::N_WARNING, Util::String("Nothing registers the action ") + this->capturing);
        }

        this->ClearCapture();
        return true;
    }

    // More than one bind can answer the same key, because a bind with no modifiers matches any
    // modifier, so the
    // most specific one is picked rather than the first one found. That is what keeps ctrl+shift+z
    // ahead of ctrl+z
    // now that the registry is ordered by the action name. Two binds that are equally specific are
    // the same combo
    // on two actions, and the tie is left to the order they are listed in
    IndexT match = InvalidIndex;
    IndexT matchMods = 0;

    for (IndexT i = 0; i < this->keybinds.Size(); i++)
    {
        const EditorKeyBind& keybind = this->keybinds[i];
        if (keybind.key == Input::Key::InvalidKey)
            continue;

        if (keybind.key == key && Input::Modifier::Matches(keybind.mods, mods))
        {
            const IndexT count = Input::Modifier::Count(keybind.mods);
            if (match == InvalidIndex || count > matchMods)
            {
                match = i;
                matchMods = count;
            }
        }
    }

    if (match == InvalidIndex)
        return false;

    this->core->ExecuteCommand(this->keybinds[match].command);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<EditorKeyBind>&
EditorKeybinds::GetKeybinds() const
{
    return this->keybinds;
}

} // namespace Editor
