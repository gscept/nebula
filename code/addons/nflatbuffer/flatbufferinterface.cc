//------------------------------------------------------------------------------
//  flatbufferinterface.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "flatbufferinterface.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "io/streamreader.h"
#include "io/uri.h"
#include "util/stringatom.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"

using namespace Flat;

namespace
{
struct FlatbufferState
{
    void Init();
    bool LoadFbs(IO::URI const& file);
    void LoadExported();
    void LoadFolder(Util::String const& folder);
    
    Util::Dictionary<Util::StringAtom, flatbuffers::Parser*> parsers;
};

void FlatbufferState::Init()
{
    n_assert(IO::AssignRegistry::HasInstance());
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("bfbs", "data:flatbuffer"));
}

void FlatbufferState::LoadFolder(Util::String const& folder)
{
    Util::Array<Util::String> files = IO::IoServer::Instance()->ListFiles(folder, "*.bfbs", true);

    for (int i = 0; i < files.Size(); i++)
    {
        LoadFbs(files[i]);
    }

    // Recurse all folders
    Util::Array<Util::String> dirs = IO::IoServer::Instance()->ListDirectories(folder, "*", true);
    for (auto const& dir : dirs)
    {
        this->LoadFolder(dir);
    }
}

void FlatbufferState::LoadExported()
{
    this->LoadFolder("bfbs:");
}

bool FlatbufferState::LoadFbs(IO::URI const& file)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    if (stream->Open())
    {
        void * buf = stream->Map();
        flatbuffers::Parser* parser = n_new(flatbuffers::Parser);
        if (parser->Deserialize((uint8_t*)buf, stream->GetSize()))
        {
            if (!parser->file_identifier_.empty())
            {
                FlatbufferState::parsers.Add(parser->file_identifier_.c_str(), parser);
                return true;
            }
        }
        n_delete(parser);
    }
    return false;
}

FlatbufferState state;
}

void FlatbufferInterface::Init()
{
    state.Init();
    state.LoadExported();
}

bool FlatbufferInterface::LoadSchema(IO::URI const& file)
{
    return state.LoadFbs(file);
}

bool FlatbufferInterface::HasSchema(Util::StringAtom ident)
{
    return state.parsers.Contains(ident);
}

Util::String FlatbufferInterface::BufferToText(const uint8_t* buf, Util::StringAtom ident)
{
    std::string output;
    flatbuffers::GenerateText(*state.parsers[ident], buf, &output);
    return output.c_str();
}

